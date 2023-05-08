 /* 
    update 2016-10-18 schmucr1: 
    expression file columns have changed, new column 7 of 8 is 'All_Mapping_Reads'
    since Biokit Version 3.9
 */
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include "format.h"
#include "log.h"
#include "linestream.h"
#include "arg.h"
#include "rofutil.h"

#define STARTUP_MSG "N/A"
#define PROG_VERSION "DEV"
#define AUTHOR_MAIL "roland.schmucki@roche.com"

static int verbose = 0;

typedef struct {
  char *sample;
  char *probe;
  char *gene;
  float rpkm;
  int count;
  int flag;
  Array rpkms;
  Array counts;
} Signal;

typedef struct {
  char *id;
  char *gene;
  char *gi;
  char *desc;
  int flag;
} Probe;

static int orderSignalsByProbe (Signal *a,Signal *b)
{
  return strcmp (a->probe,b->probe);
}

static int orderSignalsByGene (Signal *a,Signal *b)
{
  return strcmp (a->gene,b->gene);
}

/*static int orderProbesById (Probe *a,Probe *b)
{
  return strcmp (a->id,b->id);
}*/


void usagef (int level)
{
  romsg ("Description: \n"
         "\n"
         "Convert biokit expression gene count files into GCT format. \n"
         "\n"
         "Usage: %s -infile='list of files' -outfile-prefix STRING \n"
         "\n"
         "Mandatory input parameters: \n"
         "\n"
         "        -infile: either a file containing the paths to input expression files OR \n"
         "                 list of space/comma-separated files, e.g. \n"
	 "                 -infile='sample1.expression,sample2.expression' \n"
         "\n"
         "        -outfile-prefix: there are 2 - 4 output files, already existing files of same \n"
         "                         name will be overwritten:\n"
         "                         STRING_rpkm.gct \n"
         "                         STRING_count.gct \n"
         "\n"
         "Optional input parameters: \n"
         "\n"
         "       -use-unique-counts  (use the unique rpkm/read counts, default is multiple) \n"
         "\n"
         "       -old-biokit-format  (use if expression file was generated with Biokit v3.8 or \n"
	 "                           earlier; the annotation is in column #7 instead of #8) \n"
	 "\n"
	 "\n"
         "Report bugs and feedback to %s \n",
         arg_getProgName (),AUTHOR_MAIL);
}


int main(int argc, char *argv[])
{
  Signal *currSignal;
  Signal *prevSignal;
  Signal oneSignal;
  Probe *currProbe;
  //Probe oneProbe;
  int index;
  Texta it,it2;
  Texta samples;
  Texta infiles;
  int i,k;
  int sampleNumber;
  LineStream ls;
  char *line;
  Array signals;
  Array probes;
  FILE *fP1,*fP2;
  //int chipDefinitionId = 60;
  Stringa str;
  str = stringCreate (100);
  int isample = 0;
  int nvalid;
  int nfails;
  int newprobes;
  int knownprobes;
  int useUnique;

  if (arg_init (argc,argv,
                "fails,1 prefix,1 use-unique-counts,0 "
                "old-biokit-format,0 verbose,0",
                "infile outfile-prefix",
                usagef) != argc)
    die ("wrong number of arguments; invoke program without params for help");
  
  if (arg_present ("verbose"))
    verbose = 1;

  if (arg_present ("use-unique-counts"))
    useUnique = 1;
  else
    useUnique = 0;
  

  samples = textCreate (1);
  infiles = textCreate (1);
  it = textStrtokP (arg_get ("infile"),";, ");

  /* sample names from input file */
  if (arrayMax (it) == 1 && hlr_fileSizeGet (arg_get ("infile")) && 
      !strEndsWith (arg_get ("infile"),".expression")) {
    ls = ls_createFromFile (arg_get ("infile"));
    while (line = ls_nextLine (ls)) {
      it2 = textStrtokP (line,"\t");
      textAdd (infiles,textItem (it2,0));
      if (arrayMax (it2) > 1)
        stringPrintf (str,textItem (it2,1));
      else
        stringPrintf (str,textItem (it2,0));
      textDestroy (it2);
      if (strEndsWith (string (str),".expression"))
        stringChop (str,11);
      textAdd (samples,string (str));
    }
    ls_destroy (ls);
  }
  else {
    /* sample names from command line*/
    for (i=0;i<arrayMax (it);i++) {
      textAdd (infiles,textItem (it,i));
      it2 = textStrtokP (textItem (it,i),"/");
      stringPrintf (str,"%s",textItem (it2,arrayMax (it2)-1));
      if (strEndsWith (string (str),".expression"))
        stringChop (str,11);
      textAdd (samples,string (str));
      textDestroy (it2);
    }
  }
  textDestroy (it);
  sampleNumber = arrayMax (samples);

  if (verbose) {
    for (i=0;i<arrayMax (samples);i++)
      romsg ("Input file %d:\t%s\t%s",i+1,textItem (infiles,i),textItem (samples,i));
  }



  /*
    Read in all expression files with signals (=rpkm/count) and merge in 1 matrix
    Update 2016-10-18 schmucr1: 
    Expression file columns have changed, new column 7 of 8 is 'All_Mapping_Reads', since Biokit Version 3.9 
  */
  signals = arrayCreate (10000,Signal);
  probes = arrayCreate (10000,Probe);
  printf ("# Sample\tInfile\tKnown Probes\tNew Probes Added\tTotal\n");
  for (isample=0;isample<sampleNumber;isample++) {
    newprobes = 0;
    knownprobes = 0;
    printf ("# %s\t%s",textItem (samples,isample),textItem (infiles,isample));
    ls = ls_createFromFile (textItem (infiles,isample));
    while (line = ls_nextLine (ls)) {
      if (strStartsWith (line,"Gene"))
	continue;
      it = textStrtokP (line,"\t");
      if (isample == 0) {
        newprobes++;
        currSignal = arrayp (signals,arrayMax (signals),Signal);
        currSignal->probe = hlr_strdup (textItem (it,0));
        currSignal->flag = 0;
        currSignal->rpkms = arrayCreate (sampleNumber,float);
        currSignal->counts = arrayCreate (sampleNumber,int);
        // changed 2016-10-18
        if (arg_present ("old-biokit-format")) {
          if (arrayMax (it) < 7)
            die ("Missing field on line %s (7 fields are required; until Biokit Version 3.8)",line);
          currSignal->gene = hlr_strdup (textItem (it,6));
        }
        else {
          if (arrayMax (it) < 8)
              die ("Missing field on line %s (8 fields are required; since Biokit Version 3.9)",line);
          currSignal->gene = hlr_strdup (textItem (it,7));
        }
        for (i=0;i<sampleNumber;i++) {
          array (currSignal->rpkms,arrayMax (currSignal->rpkms),float) = 0.;
          array (currSignal->counts,arrayMax (currSignal->counts),int) = 0;
        }
        if (useUnique == 1) {
          arru (currSignal->rpkms,isample,float) = atof (textItem (it,3));
          arru (currSignal->counts,isample,int) = atoi (textItem (it,4));
        }
        else {
          arru (currSignal->rpkms,isample,float) = atof (textItem (it,1));
          arru (currSignal->counts,isample,int) = atoi (textItem (it,2));
        }
      }
      else {
        oneSignal.probe = textItem (it,0);
        if (arrayFind (signals,&oneSignal,&index,(ARRAYORDERF)orderSignalsByProbe)) {
          knownprobes++;
          currSignal = arrp (signals,index,Signal);
          if (useUnique == 1) {
            arru (currSignal->rpkms,isample,float) = atof (textItem (it,3));
            arru (currSignal->counts,isample,int) = atoi (textItem (it,4));
          }
          else {
            arru (currSignal->rpkms,isample,float) = atof (textItem (it,1));
            arru (currSignal->counts,isample,int) = atoi (textItem (it,2));
          }
        }
        else {
          newprobes++;
          currSignal = arrayp (signals,arrayMax (signals),Signal);
          currSignal->probe = hlr_strdup (textItem (it,0));
          currSignal->flag = 0;
          currSignal->rpkms = arrayCreate (sampleNumber,float);
          currSignal->counts = arrayCreate (sampleNumber,int);
          // changed 2016-10-18
          if (arg_present ("old-biokit-format")) {
            if (arrayMax (it) < 7)
              die ("Missing field on line %s (7 fields are required; until Biokit Version 3.8)",line);
            currSignal->gene = hlr_strdup (textItem (it,6));
          }
          else {
            if (arrayMax (it) < 8)
              die ("Missing field on line %s (8 fields are required; since Biokit Version 3.9)",line);
            currSignal->gene = hlr_strdup (textItem (it,7));
          }
          for (i=0;i<sampleNumber;i++) {
            array (currSignal->rpkms,arrayMax (currSignal->rpkms),float) = 0.;
            array (currSignal->counts,arrayMax (currSignal->counts),int) = 0;
          }
          if (useUnique == 1) {
            arru (currSignal->rpkms,isample,float) = atof (textItem (it,3));
            arru (currSignal->counts,isample,int) = atoi (textItem (it,4));
          }
          else {
            arru (currSignal->rpkms,isample,float) = atof (textItem (it,1));
            arru (currSignal->counts,isample,int) = atoi (textItem (it,2));
          }
          arraySort (signals,(ARRAYORDERF)orderSignalsByProbe);
        }
        /*        if (verbose)
          printf ("sample=%s\tknownprobes=%d\tnewprobes=%d\tsum=%d\n",
          textItem (infiles,isample),knownprobes,newprobes,knownprobes+newprobes);*/
      }
      textDestroy (it);
    }
    ls_destroy (ls);
    arraySort (signals,(ARRAYORDERF)orderSignalsByProbe);
    printf ("\t%d\t%d\t%d",knownprobes,newprobes,arrayMax (signals));
    printf ("\n");
  }
  if (verbose) {
    if (useUnique)
      printf ("# 'unique' rpkm/read counts used\n");
    else
      printf ("# 'multiple' rpkm/read counts used\n");
    printf ("# %d samples and %d probes with data read\n",
            arrayMax (samples),arrayMax (signals));
  }


  /* probes: sort and make unique of tmp */
  for (i=0;i<arrayMax (signals);i++) {
    currSignal = arrp (signals,i,Signal);
    if (i==0) {
      currProbe = arrayp (probes,arrayMax (probes),Probe);
      currProbe->id = hlr_strdup (currSignal->probe);
      currProbe->gene = hlr_strdup (currSignal->gene);
      prevSignal = arrp (signals,i,Signal);
      continue;
    }
    if (!strEqual (prevSignal->probe,currSignal->probe)) {
      currProbe = arrayp (probes,arrayMax (probes),Probe);
      currProbe->id = hlr_strdup (currSignal->probe);
      currProbe->gene = hlr_strdup (currSignal->gene);
    }
    prevSignal = arrp (signals,i,Signal);
  }
  if (verbose)
    printf ("# %d unique probes in data\n",arrayMax (probes));

  for (i=0;i<arrayMax (signals);i++) {
    currSignal = arrp (signals,i,Signal);
    if (currSignal->gene == NULL)
      printf ("%s\t%s\n",currSignal->probe,currSignal->gene);
  }


 /* output valid signals into outfile gct format */
  arraySort (signals,(ARRAYORDERF)orderSignalsByGene);
  nvalid = 0;
  nfails = 0;
  for (i=0;i<arrayMax (signals);i++) {
    currSignal = arrp (signals,i,Signal);
    if (currSignal->flag == 0)
      nvalid++;
    else
      nfails++;
  }
  if (nvalid > 0) {
    stringPrintf (str,"%s_rpkm.gct",arg_get ("outfile-prefix"));
    fP1 = hlr_fopenWrite (string (str));
    fprintf (fP1,"#1.2\n%d\t%d\n",nvalid,sampleNumber);
    
    stringPrintf (str,"%s_count.gct",arg_get ("outfile-prefix"));
    fP2 = hlr_fopenWrite (string (str));
    fprintf (fP2,"#1.2\n%d\t%d\n",nvalid,sampleNumber);
    
    stringPrintf (str,"Name\tDescription");
    for (i=0;i<arrayMax (samples);i++) 
      stringAppendf (str,"\t%s",textItem (samples,i));
    fprintf (fP1,"%s\n",string (str));
    fprintf (fP2,"%s\n",string (str));
    for (i=0;i<arrayMax (signals);i++) {
      currSignal = arrp (signals,i,Signal);
      if (currSignal->flag > 0)
        continue;
      fprintf (fP1,"%s\t%s",currSignal->probe,currSignal->gene);
      fprintf (fP2,"%s\t%s",currSignal->probe,currSignal->gene);
      for (k=0;k<arrayMax (samples);k++) {
        fprintf (fP1,"\t%f",arru (currSignal->rpkms,k,float));
        fprintf (fP2,"\t%d",arru (currSignal->counts,k,int));
      }
      fprintf (fP1,"\n");
      fprintf (fP2,"\n");
    }
    fclose (fP1);
    fclose (fP2);
  }

  /* output failed signals into outfile gct format */
  if (nfails > 0) {
    stringPrintf (str,"%s_rpkm_fails.gct",arg_get ("outfile-prefix"));
    fP1 = hlr_fopenWrite (string (str));
    fprintf (fP1,"#1.2\n%d\t%d\n",nfails,sampleNumber);
    
    stringPrintf (str,"%s_count_fails.gct",arg_get ("outfile-prefix"));
    fP2 = hlr_fopenWrite (string (str));
    fprintf (fP2,"#1.2\n%d\t%d\n",nfails,sampleNumber);
    
    stringPrintf (str,"Name\tDescription");
    for (i=0;i<arrayMax (samples);i++) 
      stringAppendf (str,"\t%s",textItem (samples,i));
    fprintf (fP1,"%s\n",string (str));
    fprintf (fP2,"%s\n",string (str));
    for (i=0;i<arrayMax (signals);i++) {
      currSignal = arrp (signals,i,Signal);
      if (currSignal->flag == 0)
        continue;
      fprintf (fP1,"%s\t%s",currSignal->probe,currSignal->gene);
      fprintf (fP2,"%s\t%s",currSignal->probe,currSignal->gene);
      for (k=0;k<arrayMax (samples);k++) {
        fprintf (fP1,"\t%f",arru (currSignal->rpkms,k,float));
        fprintf (fP2,"\t%d",arru (currSignal->counts,k,int));
      }
      fprintf (fP1,"\n");
      fprintf (fP2,"\n");
    }
    fclose (fP1);
    fclose (fP2);
  }

  return 0;
}
