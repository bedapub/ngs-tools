#include <math.h>
#include "format.h"
#include "log.h"
#include "linestream.h"
#include "arg.h"
#include "rofutil.h"

#define AUTHOR_MAIL "roland.schmucki@roche.com"

typedef struct {
  char *dir;
  char *name;
  char *group;
  int index;
} Sample;


void usagef (int level)
{
  romsg ("Description: \n"
         "\n"
         "Usage: %s [-prefix STRING] -gct FILE -i FILE \n"
         "\n"
         "Mandatory parameters: \n"
	 "\n"
         "\t-gct GCT_FILE         input file in GCT format \n"
         "\t-i   ANNOTATION_FILE  input file with sample annotations with 2 columns (see below) \n"
         "\n"
         "Optional parameters: \n"
	 "\n"
         "\t-prefix STRING        a string for the output prefix \n"
         "\n"
         "\tThe annotation file is a 2 column tab-delimited file (comments or header mark with #) \n"
         "\t  column 1: sample name as given in input GCT file \n"
         "\t  column 2: sample group \n"
         "\n"
	 "\n"
         "Report bugs and feedback to %s \n",
         arg_getProgName (),AUTHOR_MAIL);
}



int main (int argc,char *argv[])
{

  if (arg_init (argc,argv,"prefix,1","gct i",usagef) != argc)
    die ("wrong number of arguments; invoke program without params for help");
  
  LineStream ls;
  char *line;
  Texta it;
  Stringa str = stringCreate (100);
  int i,j,k;
  Texta sampleNames = textCreate (1);
  Array samples = arrayCreate (1,Sample);
  Sample *currSample;
  Texta tmp = textCreate (1);
  Texta groups = textCreate (1);
  char *prefix;
  FILE *fP;
  char *designFile;
  char *contrastFile;
 
  if (arg_present ("prefix")) {
    prefix = hlr_strdup (arg_get ("prefix"));
    stringPrintf (str,"%s_designMatrix.txt",prefix);
    designFile = hlr_strdup (string (str));
    stringPrintf (str,"%s_contrastMatrix.txt",prefix);
    contrastFile = hlr_strdup (string (str));
  }
  else {
    designFile = hlr_strdup ("designMatrix.txt");
    contrastFile = hlr_strdup ("contrastMatrix.txt");
  }

  // read gct header: samples
  stringPrintf (str,"head -3 %s",arg_get ("gct"));
  ls = ls_createFromPipe (string (str));
  while (line = ls_nextLine (ls)) {
    if (ls_lineCountGet (ls) == 3) {
      it = textStrtokP (line,"\t");
      for (i=2;i<arrayMax (it);i++)
        textAdd (sampleNames,textItem (it,i));
      textDestroy (it);
    }
  }
  ls_destroy (ls);

  // read sample annotations
  ls = ls_createFromFile (arg_get ("i"));
  while (line = ls_nextLine (ls)) {
    if (line[0] == '#')
      continue;
    it = textStrtokP (line,"\t");
    if (arrayMax (it) < 2)
      die ("missing fields in input file %s on line %s",arg_get ("i"),line);
    /* discard samples that are not in the gct file */
    for (i=0;i<arrayMax (sampleNames);i++) {
      if (strEqual (textItem (sampleNames,i),textItem (it,0)))
        break;
    }
    if (i < arrayMax (sampleNames)) {
      currSample = arrayp (samples,arrayMax (samples),Sample);
      currSample->name = hlr_strdup (textItem (it,0));
      currSample->group = hlr_strdup (textItem (it,1));
      currSample->index = -1;
      textAdd (tmp,textItem (it,1));
    }
    textDestroy (it);
  }
  ls_destroy (ls);

  if (arrayMax (tmp) == 0) 
    die ("size of tmp = %d\n",arrayMax (tmp));

  textAdd (groups,textItem (tmp,0));
  for (i=1;i<arrayMax (tmp);i++) {
    for (j=0;j<arrayMax (groups);j++) {
      if (strEqual (textItem (groups,j),textItem (tmp,i)))
        break;
    }
    if (j==arrayMax (groups))
      textAdd (groups,textItem (tmp,i));
  }
 

/*  arraySort (tmp,(ARRAYORDERF)arrayStrcmp);
  prev = textItem (tmp,0);
  textAdd (groups,prev);
  for (i=1;i<arrayMax (tmp);i++) {
    if (!strEqual (textItem (tmp,i),prev))
      textAdd (groups,textItem (tmp,i));
    prev = textItem (tmp,i);
  }
  for (i=0;i<arrayMax (samples);i++) {
    currSample = arrp (samples,i,Sample);
    for (j=0;j<arrayMax (groups);j++) {
      if (strEqual (textItem (groups,j),currSample->group)) {
        currSample->index = j;
        break;
      }
    }
  }
*/

//  for (i=0;i<arrayMax (samples);i++) {
//    currSample = arrp (samples,i,Sample);
//    romsg ("%s\t%s\t%d",currSample->name,currSample->group,currSample->index);
//  }

  // output designMatrix
  fP =  hlr_fopenWrite (designFile);
  for (i=0;i<arrayMax (groups);i++)
   fprintf (fP,"\t%s",textItem (groups,i));
  fprintf (fP,"\n");
  for (j=0;j<arrayMax (sampleNames);j++) {
    fprintf (fP,"%s",textItem (sampleNames,j));
    for (i=0;i<arrayMax (groups);i++) {
      for (k=0;k<arrayMax (samples);k++) {
        currSample = arrp (samples,k,Sample);
        if ( strEqual (textItem (sampleNames,j),currSample->name) && strEqual (textItem (groups,i),currSample->group)) {
          fprintf (fP,"\t1");
          break;
        }
      }
      if (k == arrayMax (samples))
        fprintf (fP,"\t0");
    }
    fprintf (fP,"\n");
  }
  fclose (fP);

  // output contrastMatrix
  fP =  hlr_fopenWrite (contrastFile);
  for (i=0;i<arrayMax (groups);i++) {
    for (j=i+1;j<arrayMax (groups);j++)
      fprintf (fP,"\t%s_vs_%s",textItem (groups,i),textItem (groups,j));
  }
  fprintf (fP,"\n");
  for (k=0;k<arrayMax (groups);k++) {
    fprintf (fP,"%s",textItem (groups,k));

    for (i=0;i<arrayMax (groups);i++) {
      for (j=i+1;j<arrayMax (groups);j++) {
        if (strEqual (textItem (groups,k),textItem (groups,i)))
          fprintf (fP,"\t1");
        else if (strEqual (textItem (groups,k),textItem (groups,j)))
          fprintf (fP,"\t-1");
        else
          fprintf (fP,"\t0");
      }
    }
    fprintf (fP,"\n");
  }
  fclose (fP);
  return 0;
}
