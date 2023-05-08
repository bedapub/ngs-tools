#include <math.h>
#include "format.h"
#include "log.h"
#include "linestream.h"
#include "arg.h"

#define AUTHOR_MAIL "roland.schmucki@roche.com"
#define COLS 5


typedef struct {
  char *id;
  char *group;
  //int libSize;
  int groupIndex;
  int colIndex;
} Sample;


void usagef (int level)
{
  romsg ("Description: \n"
         "\n"
	 "Calculate means per sample conditions. \n"
         "\n"
         "Usage: %s [-skip INT] [-gzip]  -i INFILE  -s SAMPLE_ANNOTATIONS \n"
	 "\n"
	 "Mandatory parameters: \n"
	 "\t  -i FILE  inptu file with sample data, e.g. read counts \n"
	 "\t  -s FILE  input file with sample annotations \n"
	 "\n"
         "The SAMPLE_ANNOTATIONS file is tab-delimited input file with at least 2 columns: \n"
         "\t column 1: sample name \n"
         "\t column 2: sample condition \n"
	 "\n"
         "The read count from INFILE are averaged (mean) for each sample condition. \n"
         "The INFILE headers should match with the sample names specified in the SAMPLE_ANNOTATIONS file. \n"
	 "Note that the header line must begin with '#' or with 'ID' \n"
	 "\n"
	 "Optional parameters: \n"
	 "\n"
         "\t  -skip INT     denotes how many columns from the INFILE should be skipped and \n"
	 "\t                not used for calculation, e.g. skip ID or description columns, default %d \n"
	 "\t  -gzip         use if INFILE is gzipped\n"
	 "\n" 
         "Report bugs and feedback to %s \n",
         arg_getProgName (),COLS+1, AUTHOR_MAIL);
}



int main (int argc,char *argv[])
{

  if (arg_init (argc,argv,"gzip,0 skip,1","i s",usagef) != argc)
    die ("wrong number of arguments; invoke program without params for help");

  
  LineStream ls;
  char *line;
  Texta it;
  Texta groups;
  int i,j;
  Array samples = arrayCreate (1,Sample);
  Sample *currSample;
  int skipCols = COLS;
  Stringa str = stringCreate (100);
  
  if (arg_present ("skip")) 
    skipCols = atoi (arg_get ("skip")) - 1;
    

  /* sample annotation file */
  groups = textCreate (1);
  ls = ls_createFromFile (arg_get ("s"));
  while (line = ls_nextLine (ls)) {
    it = textStrtokP (line,"\t");
    currSample = arrayp (samples,arrayMax (samples),Sample);
    currSample->id = hlr_strdup (textItem (it,0));
    currSample->group = hlr_strdup (textItem (it,1));
    currSample->groupIndex = -1;
    textDestroy (it);    
    if (arrayMax (groups) == 0)
      textAdd (groups,currSample->group);
    else {
      for (i=0;i<arrayMax (groups);i++) {
        if (strEqual (textItem (groups,i),currSample->group))
          break;
      }
      if (i == arrayMax (groups))
        textAdd (groups,currSample->group);
    }
  }
  ls_destroy (ls);

  /* determine group indices */
  int groupSizes[arrayMax (groups)];
  float avg[arrayMax (groups)];
  //float libSizes[arrayMax (groups)];
  for (j=0;j<arrayMax (groups);j++) {
    groupSizes[j] = 0;
    //libSizes[j] = 0.;
    avg[j] = 0.;
    for (i=0;i<arrayMax (samples);i++) {
      currSample = arrp (samples,i,Sample);
      if (strEqual (currSample->group,textItem (groups,j))) {
        currSample->groupIndex = j;
        groupSizes[j]++;
      }
     }
  }
  for (j=0;j<arrayMax (groups);j++) {
    romsg ("Group:\t%d\t%s\tSize:\t%d",j+1,textItem (groups,j),groupSizes[j]);
  }
  for (i=0;i<arrayMax (samples);i++) {
    currSample = arrp (samples,i,Sample);
    romsg ("Sample:\t%d\t%s\t%s\t%d",i+1,currSample->id,currSample->group,currSample->groupIndex);
  }


  /* parse input data file */
  if (arg_present ("gzip")) {
    stringPrintf (str, "gunzip -c %s", arg_get ("i"));
    ls = ls_createFromPipe (string (str));
  } else 
    ls = ls_createFromFile (arg_get ("i"));
  while (line = ls_nextLine (ls)) {
    /* header line */
    if (line[0] == '#' || (line[0] == 'I' && line[1] == 'D')) {
      it = textStrtokP (line,"\t");

      if (arrayMax (it) <= skipCols)
        die ("Number of columns to skip is larger then input column number");
      for (i=0;i<arrayMax (it);i++) {
        for (j=0;j<arrayMax (samples);j++) {
          currSample = arrp (samples,j,Sample);
          if (strEqual (currSample->id,textItem (it,i))) 
	    currSample->colIndex = i;
        }
	if (i > skipCols)
	  continue;
        if (i > 0)
          printf ("\t");
        printf ("%s",textItem (it,i));
      }
      textDestroy (it);
      for (j=0;j<arrayMax (groups);j++) 
	printf ("\t%s",textItem (groups,j));
      printf ("\n");
      fflush (stdout);
      continue;
    }

    /* parse data lines */
    it = textFieldtokP (line,"\t");
    for (i=0;i<arrayMax (groups);i++)
      avg[i] = 0.;
    for (i=0;i<arrayMax (samples);i++) {
      currSample = arrp (samples,i,Sample);
      if (currSample->colIndex >= arrayMax (it))
        die ("array size problem line=%s\nindex=%s\nmax=%d\n",line,currSample->colIndex,arrayMax (it));
      avg[currSample->groupIndex] += atof (textItem (it,currSample->colIndex));
    }
    for (i=0;i<arrayMax (groups);i++)
      avg[i] = avg[i] / groupSizes[i];

    /* output data: averages */
    printf ("%s",textItem (it,0));
    for (i=1;i<skipCols+1;i++) // i < 6
      printf ("\t%s",textItem (it,i));
    for (i=0;i<arrayMax (groups);i++)
      printf ("\t%.6f",avg[i]);
    printf ("\n");
    fflush (stdout);
    textDestroy (it);
  }
  ls_destroy (ls);

  return 0;
}
