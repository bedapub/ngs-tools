#include <math.h>
#include "format.h"
#include "log.h"
#include "linestream.h"
#include "arg.h"

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
         "Create a phenotype CLS file from a given GCT and annotation file. \t"
	 "The CLS format is defined here: \n"
         "https://software.broadinstitute.org/cancer/software/gsea/wiki/index.php/Data_formats#Phenotype_Data_Formats \n"
         "\n"
	 "Usage: %s -gct FILE  -i FILE \n"
	 "\n"
         "Mandatory input parameters: \n"
	 "\n"
         "\t  -gct GCT_FILE         input file in GCT format \n"
         "\t  -i   ANNOTATION_FILE  input file with sample annotations \n"
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

  if (arg_init (argc,argv,"","gct i",usagef) != argc)
    die ("wrong number of arguments; invoke program without params for help");
  
  LineStream ls;
  char *line;
  Texta it;
  Stringa str = stringCreate (100);
  int i,j;
  Texta sampleNames = textCreate (1);
  Array samples = arrayCreate (1,Sample);
  Sample *currSample;
  Texta tmp = textCreate (1);
  Texta groups = textCreate (1);
  char *prev;

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

  arraySort (tmp,(ARRAYORDERF)arrayStrcmp);
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

  for (i=0;i<arrayMax (samples);i++) {
    currSample = arrp (samples,i,Sample);
    romsg ("%s\t%s\t%d",currSample->name,currSample->group,currSample->index);
  }

  printf ("%d %d 1\n#",arrayMax (sampleNames),arrayMax (groups));
  for (i=0;i<arrayMax (groups);i++)
    printf (" %s",textItem (groups,i));
  printf ("\n");

  for (j=0;j<arrayMax (sampleNames);j++) {
    for (i=0;i<arrayMax (samples);i++) {
      currSample = arrp (samples,i,Sample);
      if (strEqual (textItem (sampleNames,j),currSample->name))
        break;
    }
    if (i == arrayMax (samples))
      die ("sample %s not found",textItem (sampleNames,j));
    if (j > 0)
      printf (" ");
    printf ("%d",currSample->index);
  }
  printf ("\n");
  
  return 0;
}
