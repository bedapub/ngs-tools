#include <stdlib.h>
#include <math.h>
#include "format.h"
#include "log.h"
#include "linestream.h"
#include "arg.h"
#include "rofutil.h"

#define AUTHOR_MAIL "roland.schmucki@roche.com"
#define BSUB_PATH "bsub"
#define SCRIPT_PREFIX "./merge_fastq"

typedef struct {
  char *in;
  char *out;
} Item;

static int orderItemsByOut (Item *a,Item *b)
{
  return strcmp (a->out,b->out);
}


void usagef (int level)
{
  romsg ("Description: \n"
         "\n"
         "Merge reads from several fastq files into one fastq file. \n"
	 "IMPORTANT: input files for mate R1 and R2 reads must be in the same ORDER. \n"
	 "\n"
         "Usage: %s [-sbatch] [-t INT] [-old-version] [-script-prefix STR] [-bsub-path STR] -i FILE \n"
	 "\n"
         "Mandatory parameters: \n"
	 "\n"
         "  -i  input_file  tab-delimited file with 2 columns: input gzipped fastq file, \n"
	 "                  output gzipped fastq file \n"
         "\n"
	 "\n"
         "Optional parameters: \n"
	 "\n"
         "  -sbatch         use \"sbatch\" for submitting to queue, default is \"bsub\" \n"
         "  -t  integer     number of minutes for queuing system, default 360 = 6 hours, only for \"sbatch\" \n"
         "  -old-version    use old version which is much slower \n"
         "  -script-prefix  prefix for temp scripts, e.g. path, default %s \n"
	 "  -bsub-path      path to bsub command on the shpc, default %s \n"
         "\n"
	 "\n"
         "Report bugs and feedback to %s \n",
         arg_getProgName (), SCRIPT_PREFIX, BSUB_PATH, AUTHOR_MAIL);
}



int main (int argc,char *argv[])
{

  if (arg_init (argc,argv,"bsub-path,1 script-prefix,1 sbatch,0 old-version,0 t,1","i",usagef) != argc)
    die ("wrong number of arguments; invoke program without params for help");
  
  LineStream ls;
  char *line;
  Texta it;
  Array items = arrayCreate (10,Item);
  Item *currItem;
  Item *prevItem;
  Texta outs = textCreate (10);
  int i,j;
  Stringa str = stringCreate (100);
  FILE *fP;
  char *name = NULL;
  int timeMinutes = 360;
  char *scriptPrefix = NULL;
  char *scriptName = NULL;
  char *outdir = NULL;
  char *outfile = NULL;
  char *bsubPath = NULL;
	
  if (arg_present ("t"))
    timeMinutes = atoi (arg_get ("t"));
  
  if (arg_present ("script-prefix"))
    strReplace (&scriptPrefix, arg_get ("script-prefix"));
  else
    strReplace (&scriptPrefix, SCRIPT_PREFIX);
    
  if (arg_present ("bsub-path"))
    strReplace (&bsubPath, arg_get ("bsub-path"));
  else
    strReplace (&bsubPath, BSUB_PATH); 
	
  ls = ls_createFromFile (arg_get ("i"));
  while (line = ls_nextLine (ls)) {
    if (line[0] == '#')
      continue;
    it = textStrtokP (line,"\t");
    if (arrayMax (it) < 2)
      die ("Missing fields on line %s (2 fields required)",line);
    currItem = arrayp (items,arrayMax (items),Item);
    currItem->in = hlr_strdup (textItem (it,0));
    currItem->out = hlr_strdup (textItem (it,1));
    textDestroy (it);
  }
  ls_destroy (ls);

  arraySort (items,(ARRAYORDERF)orderItemsByOut);

  prevItem = arrp (items,0,Item);
  textAdd (outs,prevItem->out);
  for (i=1;i<arrayMax (items);i++) {
    currItem = arrp (items,i,Item);
    if (!strEqual (prevItem->out,currItem->out))
      textAdd (outs,currItem->out);
    prevItem = arrp (items,i,Item);
  }
  
  for (i=0;i<arrayMax (outs);i++) {
    strReplace (&name, hlr_tail(textItem (outs,i)));
    stringPrintf (str,"%s_%s.sh", scriptPrefix,name);
    strReplace (&scriptName,  string(str));

    // get directory of output file names
    Texta it = textStrtokP (textItem (outs,i), "/");
    strReplace (&outfile, textItem (it, arrayMax (it)-1));
    if (arrayMax (it) == 1) 
      strReplace (&outdir, realpath (".", NULL));
    else {
      stringPrintf (str, textItem (outs,i));
      stringChop (str, strlen (textItem (it, arrayMax (it)-1)));
      strReplace (&outdir, realpath (string (str), NULL));
    }
    textDestroy (it);
    printf ("OUTDIR=%s\tOUTFILE=%s\n", outdir, outfile);

    fP =  hlr_fopenWrite (scriptName);
    if (arg_present ("old-version"))
      fprintf (fP,"#!/bin/bash\n\numask 2\n\nfor i in");
    else
      fprintf (fP,"#!/bin/bash\n\numask 2\n\ncat ");
    for (j=0;j<arrayMax (items);j++) {
      currItem = arrp (items,j,Item);
      if (!strEqual (currItem->out,textItem (outs,i)))
        continue;
      fprintf (fP," %s", realpath(currItem->in, NULL));
      printf ("append %s to %s\n", currItem->in, textItem (outs,i));
    }
    printf ("#\n");
    if (arg_present ("old-version"))
      fprintf (fP,"; do\n  gunzip -c $i | gzip -c >> %s/%s\ndone\n", outdir, outfile);
    else
      fprintf (fP," > %s/%s\n", outdir, outfile);
    fclose (fP);
    if (arg_present ("sbatch"))
      stringPrintf (str,"chmod +x %s && sbatch --time=%d -J merge_fastq_%s -e merge_fastq_%s.err "
                    "-o merge_fastq_%s.out %s",
		    scriptName,timeMinutes,name,name,name,scriptName);
    else
      stringPrintf (str,"%s -q preempt -J merge_fastq_%s -e merge_fastq_%s.err "
                    "-o merge_fastq_%s.out source %s",
		    bsubPath,name,name,name,scriptName);
    hlr_system (string (str),1);
  }
  return 0;
}
