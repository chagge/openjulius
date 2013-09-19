/**
 * @file   output_module.c
 * 
 * <JA>
 * @brief  Ç§¼±·ë²Ì¤ò¥½¥±¥Ã¥È¤Ø½ĞÎÏ¤¹¤ë. 
 * </JA>
 * 
 * <EN>
 * @brief  Output recoginition result via module socket.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Tue Sep 06 14:46:49 2005
 *
 * $Revision: 1.12 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include "app.h"

#include <time.h>

extern int module_sd;
extern boolean separate_score_flag;

/**********************************************************************/
/* process online/offline status  */

/** 
 * <JA>
 * Ç§¼±²ÄÇ½¤Ê¾õÂÖ¤Ë¤Ê¤Ã¤¿¤È¤­¤Ë¸Æ¤Ğ¤ì¤ë
 * 
 * </JA>
 * <EN>
 * Called when it becomes ready to recognize the input.
 * 
 * </EN>
 */
static void
status_process_online(Recog *recog, void *dummy)
{
  module_send(module_sd, "<STARTPROC/>\n.\n");
}
/** 
 * <JA>
 * Ç§¼±¤ò°ì»şÃæÃÇ¾õÂÖ¤Ë¤Ê¤Ã¤¿¤È¤­¤Ë¸Æ¤Ğ¤ì¤ë
 * 
 * </JA>
 * <EN>
 * Called when process paused and recognition is stopped.
 * 
 * </EN>
 */
static void
status_process_offline(Recog *recog, void *dummy)
{
  module_send(module_sd, "<STOPPROC/>\n.\n");
}

/**********************************************************************/
/* decode outcode "WLPSwlps" to each boolean value */
/* default: "WLPS" */
static boolean out1_word = FALSE, out1_lm = FALSE, out1_phone = FALSE, out1_score = FALSE;
static boolean out2_word = TRUE, out2_lm = TRUE, out2_phone = TRUE, out2_score = TRUE;
static boolean out1_never = TRUE, out2_never = FALSE;
#ifdef CONFIDENCE_MEASURE
static boolean out2_cm = TRUE;
#endif

/** 
 * <JA>
 * Ç§¼±·ë²Ì¤È¤·¤Æ¤É¤¦¤¤¤Ã¤¿Ã±¸ì¾ğÊó¤ò½ĞÎÏ¤¹¤ë¤«¤ò¥»¥Ã¥È¤¹¤ë¡£
 * 
 * @param str [in] ½ĞÎÏ¹àÌÜ»ØÄêÊ¸»úÎó ("WLPSCwlps"¤Î°ìÉô)
 * </JA>
 * <EN>
 * Setup which word information to be output as a recognition result.
 * 
 * @param str [in] output selection string (part of "WLPSCwlps")
 * </EN>
 */
void
decode_output_selection(char *str)
{
  int i;
  out1_word = out1_lm = out1_phone = out1_score = FALSE;
  out2_word = out2_lm = out2_phone = out2_score = FALSE;
#ifdef CONFIDENCE_MEASURE
  out2_cm = FALSE;
#endif
  for(i = strlen(str) - 1; i >= 0; i--) {
    switch(str[i]) {
    case 'W': out2_word  = TRUE; break;
    case 'L': out2_lm    = TRUE; break;
    case 'P': out2_phone = TRUE; break;
    case 'S': out2_score = TRUE; break;
    case 'w': out1_word  = TRUE; break;
    case 'l': out1_lm    = TRUE; break;
    case 'p': out1_phone = TRUE; break;
    case 's': out1_score = TRUE; break;
#ifdef CONFIDENCE_MEASURE
    case 'C': out2_cm    = TRUE; break;
#endif
    default:
      fprintf(stderr, "Error: unknown outcode `%c', ignored\n", str[i]);
      break;
    }
  }
  out1_never = ! (out1_word | out1_lm | out1_phone | out1_score);
  out2_never = ! (out2_word | out2_lm | out2_phone | out2_score
#ifdef CONFIDENCE_MEASURE
		  | out2_cm
#endif
		  );

}

/** 
 * <JA>
 * Ç§¼±Ã±¸ì¤Î¾ğÊó¤ò½ĞÎÏ¤¹¤ë¥µ¥Ö¥ë¡¼¥Á¥ó¡ÊÂè1¥Ñ¥¹ÍÑ¡Ë. 
 * 
 * @param w [in] Ã±¸ìID
 * @param winfo [in] Ã±¸ì¼­½ñ
 * </JA>
 * <EN>
 * Subroutine to output information of a recognized word at 1st pass.
 * 
 * @param w [in] word ID
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
msock_word_out1(WORD_ID w, RecogProcess *r)
{
  int j;
  static char buf[MAX_HMMNAME_LEN];
  WORD_INFO *winfo;

  winfo = r->lm->winfo;

  if (out1_word) {
    module_send(module_sd, " WORD=\"%s\"", winfo->woutput[w]);
  }
  if (out1_lm) {
    module_send(module_sd, " CLASSID=\"%s\"", winfo->wname[w]);
  }
  if (out1_phone) {
    module_send(module_sd, " PHONE=\"");
    for(j=0;j<winfo->wlen[w];j++) {
      center_name(winfo->wseq[w][j]->name, buf);
      if (j == 0) module_send(module_sd, "%s", buf);
      else module_send(module_sd, " %s", buf);
    }
    module_send(module_sd, "\"");
  }
}

/** 
 * <JA>
 * Ç§¼±Ã±¸ì¤Î¾ğÊó¤ò½ĞÎÏ¤¹¤ë¥µ¥Ö¥ë¡¼¥Á¥ó¡ÊÂè2¥Ñ¥¹ÍÑ¡Ë. 
 * 
 * @param w [in] Ã±¸ìID
 * @param winfo [in] Ã±¸ì¼­½ñ
 * </JA>
 * <EN>
 * Subroutine to output information of a recognized word at 2nd pass.
 * 
 * @param w [in] word ID
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
msock_word_out2(WORD_ID w, RecogProcess *r)
{
  int j;
  static char buf[MAX_HMMNAME_LEN];
  WORD_INFO *winfo;

  winfo = r->lm->winfo;

  if (out2_word) {
    module_send(module_sd, " WORD=\"%s\"", winfo->woutput[w]);
  }
  if (out2_lm) {
    module_send(module_sd, " CLASSID=\"%s\"", winfo->wname[w]);
  }
  if (out2_phone) {
    module_send(module_sd, " PHONE=\"");
    for(j=0;j<winfo->wlen[w];j++) {
      center_name(winfo->wseq[w][j]->name, buf);
      if (j == 0) module_send(module_sd, "%s", buf);
      else module_send(module_sd, " %s", buf);
    }
    module_send(module_sd, "\"");
  }
}


/**********************************************************************/
/* 1st pass output */

/** 
 * <JA>
 * Âè1¥Ñ¥¹¡§²»À¼Ç§¼±¤ò³«»Ï¤¹¤ëºİ¤Î½ĞÎÏ¡Ê²»À¼ÆşÎÏ³«»Ï»ş¤Ë¸Æ¤Ğ¤ì¤ë¡Ë. 
 * 
 * </JA>
 * <EN>
 * 1st pass: output when recognition begins (will be called at input start).
 * 
 * </EN>
 */
static void
status_pass1_begin(Recog *recog, void *dummy)
{
  module_send(module_sd, "<STARTRECOG/>\n.\n");
}

/** 
 * <JA>
 * Âè1¥Ñ¥¹¡§ÅÓÃæ·ë²Ì¤ò½ĞÎÏ¤¹¤ë¡ÊÂè1¥Ñ¥¹¤Î°ìÄê»ş´Ö¤´¤È¤Ë¸Æ¤Ğ¤ì¤ë¡Ë
 * 
 * @param t [in] ¸½ºß¤Î»ş´Ö¥Õ¥ì¡¼¥à
 * @param seq [in] ¸½ºß¤Î°ì°Ì¸õÊäÃ±¸ìÎó
 * @param num [in] @a seq ¤ÎÄ¹¤µ
 * @param score [in] ¾åµ­¤Î¤³¤ì¤Ş¤Ç¤ÎÎßÀÑ¥¹¥³¥¢
 * @param LMscore [in] ¾åµ­¤ÎºÇ¸å¤ÎÃ±¸ì¤Î¿®ÍêÅÙ
 * @param winfo [in] Ã±¸ì¼­½ñ
 * </JA>
 * <EN>
 * 1st pass: output current result while search (called periodically while 1st pass).
 * 
 * @param t [in] current time frame
 * @param seq [in] current best word sequence at time @a t.
 * @param num [in] length of @a seq.
 * @param score [in] accumulated score of the current best sequence at @a t.
 * @param LMscore [in] confidence score of last word on the sequence
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
result_pass1_current(Recog *recog, void *dummy)
{
  int i;
  WORD_INFO *winfo;
  WORD_ID *seq;
  int num;
  RecogProcess *r;
  boolean multi;

  if (out1_never) return;	/* no output specified */

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (! r->have_interim) continue;

    winfo = r->lm->winfo;
    seq = r->result.pass1.word;
    num = r->result.pass1.word_num;

    if (multi) {
      module_send(module_sd, "<RECOGOUT ID=\"SR%02d\" NAME=\"%s\">\n", r->config->id, r->config->name);
    } else {
      module_send(module_sd, "<RECOGOUT>\n");
    }
    if (out1_score) {
      module_send(module_sd, "  <PHYPO PASS=\"1\" SCORE=\"%f\" FRAME=\"%d\" TIME=\"%ld\"/>\n", r->result.pass1.score, r->result.num_frame, time(NULL));
    } else {
      module_send(module_sd, "  <PHYPO PASS=\"1\" FRAME=\"%d\" TIME=\"%ld\"/>\n", r->result.num_frame, time(NULL));
    }
    for (i=0;i<num;i++) {
      module_send(module_sd, "    <WHYPO");
      msock_word_out1(seq[i], r);
      module_send(module_sd, "/>\n");
    }
    module_send(module_sd, "  </PHYPO>\n</RECOGOUT>\n.\n");
  }
}

/** 
 * <JA>
 * Âè1¥Ñ¥¹¡§½ªÎ»»ş¤ËÂè1¥Ñ¥¹¤Î·ë²Ì¤ò½ĞÎÏ¤¹¤ë¡ÊÂè1¥Ñ¥¹½ªÎ»¸å¡¢Âè2¥Ñ¥¹¤¬
 * »Ï¤Ş¤ëÁ°¤Ë¸Æ¤Ğ¤ì¤ë. Ç§¼±¤Ë¼ºÇÔ¤·¤¿¾ì¹ç¤Ï¸Æ¤Ğ¤ì¤Ê¤¤¡Ë. 
 * 
 * @param seq [in] Âè1¥Ñ¥¹¤Î1°Ì¸õÊä¤ÎÃ±¸ìÎó
 * @param num [in] ¾åµ­¤ÎÄ¹¤µ
 * @param score [in] 1°Ì¤ÎÎßÀÑ²¾Àâ¥¹¥³¥¢
 * @param LMscore [in] @a score ¤Î¤¦¤Á¸À¸ì¥¹¥³¥¢
 * @param winfo [in] Ã±¸ì¼­½ñ
 * </JA>
 * <EN>
 * 1st pass: output final result of the 1st pass (will be called just after
 * the 1st pass ends and before the 2nd pass begins, and will not if search
 * failed).
 * 
 * @param seq [in] word sequence of the best hypothesis at the 1st pass.
 * @param num [in] length of @a seq.
 * @param score [in] accumulated hypothesis score of @a seq.
 * @param LMscore [in] language score in @a score.
 * @param winfo [in] word dictionary.
 * </EN>
 */
static void
result_pass1_final(Recog *recog, void *dummy)
{
  int i;
  RecogProcess *r;
  boolean multi;

  if (out1_never) return;	/* no output specified */

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (r->result.status < 0) continue;	/* search already failed  */

    if (multi) {
      module_send(module_sd, "<RECOGOUT ID=\"SR%02d\" NAME=\"%s\">\n", r->config->id, r->config->name);
    } else {
      module_send(module_sd, "<RECOGOUT>\n");
    }
    if (out1_score) {
      module_send(module_sd, "  <SHYPO PASS=\"1\" SCORE=\"%f\">\n", r->result.pass1.score);
    } else {
      module_send(module_sd, "  <SHYPO PASS=\"1\">\n", r->result.pass1.score);
    }
    for (i=0;i<r->result.pass1.word_num;i++) {
      module_send(module_sd, "    <WHYPO");
      msock_word_out1(r->result.pass1.word[i], r);
      module_send(module_sd, "/>\n");
    }
    module_send(module_sd, "  </SHYPO>\n</RECOGOUT>\n.\n");
  }
}

/** 
 * <JA>
 * Âè1¥Ñ¥¹¡§½ªÎ»»ş¤Î½ĞÎÏ¡ÊÂè1¥Ñ¥¹¤Î½ªÎ»»ş¤ËÉ¬¤º¸Æ¤Ğ¤ì¤ë¡Ë
 * 
 * </JA>
 * <EN>
 * 1st pass: end of output (will be called at the end of the 1st pass).
 * 
 * </EN>
 */
static void
status_pass1_end(Recog *recog, void *dummy)
{
  module_send(module_sd, "<ENDRECOG/>\n.\n");
}

/**********************************************************************/
/* 2nd pass output */

/** 
 * <JA>
 * Âè2¥Ñ¥¹¡§ÆÀ¤é¤ì¤¿Ê¸²¾Àâ¸õÊä¤ò1¤Ä½ĞÎÏ¤¹¤ë. 
 * 
 * @param hypo [in] ÆÀ¤é¤ì¤¿Ê¸²¾Àâ
 * @param rank [in] @a hypo ¤Î½ç°Ì
 * @param winfo [in] Ã±¸ì¼­½ñ
 * </JA>
 * <EN>
 * 2nd pass: output a sentence hypothesis found in the 2nd pass.
 * 
 * @param hypo [in] sentence hypothesis to be output
 * @param rank [in] rank of @a hypo
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
result_pass2(Recog *recog, void *dummy)
{
  int i, n, num;
  WORD_INFO *winfo;
  WORD_ID *seq;
  int seqnum;
  Sentence *s;
  RecogProcess *r;
  boolean multi;
  SentenceAlign *align;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;

    if (r->result.status < 0) {
      switch(r->result.status) {
      case J_RESULT_STATUS_REJECT_POWER:
	module_send(module_sd, "<REJECTED REASON=\"by power\"");
	break;
      case J_RESULT_STATUS_TERMINATE:
	module_send(module_sd, "<REJECTED REASON=\"input terminated by request\"");
	break;
      case J_RESULT_STATUS_ONLY_SILENCE:
	module_send(module_sd, "<REJECTED REASON=\"result has pause words only\"");
	break;
      case J_RESULT_STATUS_REJECT_GMM:
	module_send(module_sd, "<REJECTED REASON=\"by GMM\"");
	break;
      case J_RESULT_STATUS_REJECT_SHORT:
	module_send(module_sd, "<REJECTED REASON=\"too short input\"");
	break;
      case J_RESULT_STATUS_REJECT_LONG:
	module_send(module_sd, "<REJECTED REASON=\"too long input\"");
	break;
      case J_RESULT_STATUS_FAIL:
	module_send(module_sd, "<RECOGFAIL");
	break;
      }
      if (multi) {
	module_send(module_sd, " ID=\"SR%02d\" NAME=\"%s\"", r->config->id, r->config->name);
      }
      module_send(module_sd, "/>\n.\n");
      continue;
    }

    if (out2_never) continue;	/* no output specified */

    winfo = r->lm->winfo;
    num = r->result.sentnum;

    if (multi) {
      module_send(module_sd, "<RECOGOUT ID=\"SR%02d\" NAME=\"%s\">\n", r->config->id, r->config->name);
    } else {
      module_send(module_sd, "<RECOGOUT>\n");
    }
    for(n=0;n<num;n++) {
      s = &(r->result.sent[n]);
      seq = s->word;
      seqnum = s->word_num;
      
      module_send(module_sd, "  <SHYPO RANK=\"%d\"", n+1);
      if (out2_score) {
#ifdef USE_MBR
	if(r->config->mbr.use_mbr == TRUE){

	  module_send(module_sd, " MBRSCORE=\"%f\"", s->score_mbr);
	}
#endif
	module_send(module_sd, " SCORE=\"%f\"", s->score);
	if (r->lmtype == LM_PROB) {
	  if (separate_score_flag) {
	    module_send(module_sd, " AMSCORE=\"%f\" LMSCORE=\"%f\"", s->score_am, s->score_lm);
	  }
	}
      }
      if (r->lmtype == LM_DFA) {
	/* output which grammar the best hypothesis belongs to */
	module_send(module_sd, " GRAM=\"%d\"", s->gram_id);
      }
      
      module_send(module_sd, ">\n");
      for (i=0;i<seqnum;i++) {
	module_send(module_sd, "    <WHYPO");
	msock_word_out2(seq[i], r);
#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
	/* currently not handle multiple alpha output */
#else
	if (out2_cm) {
	  module_send(module_sd, " CM=\"%5.3f\"", s->confidence[i]);
	}
#endif
#endif /* CONFIDENCE_MEASURE */
	/* output alignment result if exist */
	for (align = s->align; align; align = align->next) {
	  switch(align->unittype) {
	  case PER_WORD:	/* word alignment */
	    module_send(module_sd, " BEGINFRAME=\"%d\" ENDFRAME=\"%d\"", align->begin_frame[i], align->end_frame[i]);
	    break;
	  case PER_PHONEME:
	  case PER_STATE:
	    fprintf(stderr, "Error: \"-palign\" and \"-salign\" does not supported for module output\n");
	    break;
	  }
	}
	
	module_send(module_sd, "/>\n");
      }
      module_send(module_sd, "  </SHYPO>\n");
    }
    module_send(module_sd, "</RECOGOUT>\n.\n");

  }

}


/**********************************************************************/
/* word graph output */

/** 
 * <JA>
 * ÆÀ¤é¤ì¤¿Ã±¸ì¥°¥é¥ÕÁ´ÂÎ¤ò½ĞÎÏ¤¹¤ë. 
 * 
 * @param root [in] ¥°¥é¥ÕÃ±¸ì½¸¹ç¤ÎÀèÆ¬Í×ÁÇ¤Ø¤Î¥İ¥¤¥ó¥¿
 * @param winfo [in] Ã±¸ì¼­½ñ
 * </JA>
 * <EN>
 * Output the whole word graph.
 * 
 * @param root [in] pointer to the first element of graph words
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
result_graph(Recog *recog, void *dummy)
{
  WordGraph *wg;
  int i;
  int nodenum, arcnum;
  WORD_INFO *winfo;
  WordGraph *root;
  RecogProcess *r;
  boolean multi;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (r->result.wg == NULL) continue;	/* no graph obtained */

    winfo = r->lm->winfo;
    root = r->result.wg;
    nodenum = r->graph_totalwordnum;
    arcnum = 0;
    for(wg=root;wg;wg=wg->next) {
      arcnum += wg->rightwordnum;
    }

    module_send(module_sd, "<GRAPHOUT");
    if (multi) module_send(module_sd, " ID=\"SR%02d\" NAME=\"%s\"", r->config->id, r->config->name);
    module_send(module_sd, " NODENUM=\"%d\" ARCNUM=\"%d\">\n", nodenum, arcnum);
    for(wg=root;wg;wg=wg->next) {
      module_send(module_sd, "    <NODE GID=\"%d\"", wg->id);
      msock_word_out2(wg->wid, r);
      module_send(module_sd, " BEGIN=\"%d\"", wg->lefttime);
      module_send(module_sd, " END=\"%d\"", wg->righttime);
      module_send(module_sd, "/>\n");
    }
    for(wg=root;wg;wg=wg->next) {
      for(i=0;i<wg->rightwordnum;i++) {
	module_send(module_sd, "    <ARC FROM=\"%d\" TO=\"%d\"/>\n", wg->id, wg->rightword[i]->id);
      }
    }
    module_send(module_sd, "</GRAPHOUT>\n.\n");
  }
}

/** 
 * <JA>
 * Â½Ã ÃˆÃ·Â¤Â¬Â½ÂªÃÂ»Â¤Â·Â¤Ã†Â¡Â¢Ã‡Â§Â¼Â±Â²Ã„Ã‡Â½Â¾ÃµÃ‚Ã–Â¡ÃŠÃ†Ã¾ÃÃÃ‚Ã”Â¤ÃÂ¾ÃµÃ‚Ã–Â¡Ã‹Â¤Ã‹Ã†Ã¾Â¤ÃƒÂ¤Â¿Â¤ÃˆÂ¤Â­Â¤ÃÂ½ÃÃÃ
 * 
 * </JA>
 * <EN>
 * Output the obtained confusion network.
 *
 * </EN>
 */
static void
result_confnet(Recog *recog, void *dummy)
{
  CN_CLUSTER *c;
  int i;
  RecogProcess *r;
  boolean multi;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (r->result.confnet == NULL) continue;  /* no confnet obtained */

    module_send(module_sd, "<CONFNET>\n");
    for(c=r->result.confnet;c;c=c->next) {
      module_send(module_sd, "  <WORD>\n");
      for(i=0;i<c->wordsnum;i++) {
        if (c->pp[i] >= 0.001)
          module_send(module_sd, "    <ALTERNATIVE PROB=\"%.3f\">%s</ALTERNATIVE>\n", c->pp[i], (c->words[i] == WORD_INVALID) ? "" : r->lm->winfo->woutput[c->words[i]]);
      }
      module_send(module_sd, "  </WORD>\n");
    }
    module_send(module_sd, "</CONFNET>\n");
  }
}

/**
 * <JA>
 * æº–å‚™ãŒçµ‚äº†ã—ã¦ã€èªè­˜å¯èƒ½çŠ¶æ…‹ï¼ˆå…¥åŠ›å¾…ã¡çŠ¶æ…‹ï¼‰ã«å…¥ã£ãŸã¨ãã®å‡ºåŠ›
 *
 * </JA>
 * <EN>
 * Output when ready to recognize and start waiting speech input.
 * 
 * </EN>
 */
static void
status_recready(Recog *recog, void *dummy)
{
  module_send(module_sd, "<INPUT STATUS=\"LISTEN\" TIME=\"%ld\"/>\n.\n", time(NULL));
}

/** 
 * <JA>
 * ÆşÎÏ¤Î³«»Ï¤ò¸¡½Ğ¤·¤¿¤È¤­¤Î½ĞÎÏ
 * 
 * </JA>
 * <EN>
 * Output when input starts.
 * 
 * </EN>
 */
static void
status_recstart(Recog *recog, void *dummy)
{
  module_send(module_sd, "<INPUT STATUS=\"STARTREC\" TIME=\"%ld\"/>\n.\n", time(NULL));
}
/** 
 * <JA>
 * ÆşÎÏ½ªÎ»¤ò¸¡½Ğ¤·¤¿¤È¤­¤Î½ĞÎÏ
 * 
 * </JA>
 * <EN>
 * Output when input ends.
 * 
 * </EN>
 */
static void
status_recend(Recog *recog, void *dummy)
{
  module_send(module_sd, "<INPUT STATUS=\"ENDREC\" TIME=\"%ld\"/>\n.\n", time(NULL));
}
/** 
 * <JA>
 * ÆşÎÏÄ¹¤Ê¤É¤ÎÆşÎÏ¥Ñ¥é¥á¡¼¥¿¾ğÊó¤ò½ĞÎÏ. 
 * 
 * @param param [in] ÆşÎÏ¥Ñ¥é¥á¡¼¥¿¹½Â¤ÂÎ
 * </JA>
 * <EN>
 * Output input parameter status such as length.
 * 
 * @param param [in] input parameter structure
 * </EN>
 */
static void
status_param(Recog *recog, void *dummy)
{
  MFCCCalc *mfcc;
  boolean multi;
  int frames;
  int msec;

  if (recog->mfcclist->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {
    frames = mfcc->param->samplenum;
    msec = (float)mfcc->param->samplenum * (float)recog->jconf->input.period * (float)recog->jconf->input.frameshift / 10000.0;
    if (multi) {
      module_send(module_sd, "<INPUTPARAM MFCCID=\"%02d\" FRAMES=\"%d\" MSEC=\"%d\"/>\n.\n", mfcc->id, frames, msec);
    } else {
      module_send(module_sd, "<INPUTPARAM FRAMES=\"%d\" MSEC=\"%d\"/>\n.\n", frames, msec);
    }
  }
}

/********************* RESULT OUTPUT FOR GMM *************************/
/** 
 * <JA>
 * GMM¤Î·×»»·ë²Ì¤ò¥â¥¸¥å¡¼¥ë¤Î¥¯¥é¥¤¥¢¥ó¥È¤ËÁ÷¿®¤¹¤ë ("-result msock" ÍÑ)
 * </JA>
 * <EN>
 * Send the result of GMM computation to module client.
 * (for "-result msock" option)
 * </EN>
 */
static void
result_gmm(Recog *recog, void *dummy)
{
  module_send(module_sd, "<GMM RESULT=\"%s\"", recog->gc->max_d->name);
#ifdef CONFIDENCE_MEASURE
  module_send(module_sd, " CMSCORE=\"%f\"", recog->gc->gmm_max_cm);
#endif
  module_send(module_sd, "/>\n.\n");
}

/** 
 * <JA>
 * ¸½ºß¤ÎÊİ»ı¤·¤Æ¤¤¤ëÊ¸Ë¡¤Î¥ê¥¹¥È¤ò¥â¥¸¥å¡¼¥ë¤ËÁ÷¿®¤¹¤ë. 
 * 
 * </JA>
 * <EN>
 * Send current list of grammars to module client.
 * 
 * </EN>
 */
void
send_gram_info(RecogProcess *r)
{
  MULTIGRAM *m;
  char buf[1024];

  if (r->lmtype == LM_PROB) {
    module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"NOT A GRAMMAR-BASED LM\"/>\n.\n");
    return;
  }
  module_send(module_sd, "<GRAMINFO>\n");
  for(m=r->lm->grammars;m;m=m->next) {
    buf[0] = '\0';
    if (m->dfa) {
      snprintf(buf, 1024, ", %3d categories, %4d nodes",
	       m->dfa->term_num, m->dfa->state_num);
    }
    if (m->newbie) strcat(buf, " (new)");
    if (m->hook != 0) {
      strcat(buf, " (next:");
      if (m->hook & MULTIGRAM_DELETE) {
	strcat(buf, " delete");
      }
      if (m->hook & MULTIGRAM_ACTIVATE) {
	strcat(buf, " activate");
      }
      if (m->hook & MULTIGRAM_DEACTIVATE) {
	strcat(buf, " deactivate");
      }
      if (m->hook & MULTIGRAM_MODIFIED) {
	strcat(buf, " modified");
      }
      strcat(buf, ")");
    }
    module_send(module_sd, "  #%2d: [%-11s] %4d words%s \"%s\"\n",
		m->id,
		m->active ? "active" : "inactive",
		m->winfo->num,
		buf,
		m->name);
  }
  if (r->lm->dfa != NULL) {
    module_send(module_sd, "  Global:            %4d words, %3d categories, %4d nodes\n", r->lm->winfo->num, r->lm->dfa->term_num, r->lm->dfa->state_num);
  }
  module_send(module_sd, "</GRAMINFO>\n.\n");
}

/**********************************************************************/
/* register functions for module output */
/** 
 * <JA>
 * ¥â¥¸¥å¡¼¥ë½ĞÎÏ¤ò¹Ô¤¦¤è¤¦´Ø¿ô¤òÅĞÏ¿¤¹¤ë. 
 * 
 * </JA>
 * <EN>
 * Register output functions to enable module output.
 * 
 * </EN>
 */
void
setup_output_msock(Recog *recog, void *data)
{
  callback_add(recog, CALLBACK_EVENT_PROCESS_ONLINE, status_process_online, data);
  callback_add(recog, CALLBACK_EVENT_PROCESS_OFFLINE, status_process_offline, data);
  //callback_add(recog, CALLBACK_EVENT_STREAM_BEGIN,     , data);
  //callback_add(recog, CALLBACK_EVENT_STREAM_END,        , data);
  callback_add(recog, CALLBACK_EVENT_SPEECH_READY, status_recready, data);
  callback_add(recog, CALLBACK_EVENT_SPEECH_START, status_recstart, data);
  callback_add(recog, CALLBACK_EVENT_SPEECH_STOP, status_recend, data);
  callback_add(recog, CALLBACK_EVENT_PASS1_BEGIN, status_pass1_begin, data);
  callback_add(recog, CALLBACK_EVENT_PASS1_END, status_pass1_end, data);
  callback_add(recog, CALLBACK_RESULT_PASS1_INTERIM, result_pass1_current, data);
  callback_add(recog, CALLBACK_RESULT_PASS1, result_pass1_final, data);

  callback_add(recog, CALLBACK_STATUS_PARAM, status_param, data);

  callback_add(recog, CALLBACK_RESULT, result_pass2, data); // rejected, failed
  callback_add(recog, CALLBACK_RESULT_GMM, result_gmm, data);
  /* below will not be called if "-graphout" not specified */
  callback_add(recog, CALLBACK_RESULT_GRAPH, result_graph, data);
  /* below will be called when "-confnet" is specified */
  callback_add(recog, CALLBACK_RESULT_CONFNET, result_confnet, data);

  //callback_add(recog, CALLBACK_EVENT_PAUSE, status_pause, data);
  //callback_add(recog, CALLBACK_EVENT_RESUME, status_resume, data);

}
