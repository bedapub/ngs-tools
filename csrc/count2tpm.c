#include <math.h>
#include "format.h"
#include "log.h"
#include "linestream.h"
#include "arg.h"
#include "array.h"

#define STARTUP_MSG "N/A"
#define PROG_VERSION "DEV"
#define AUTHOR_MAIL "roland.schmucki@roche.com"
#define DIGITS 3

typedef struct {
  int len;
  int flag;
  char *id;
  char *desc;
  Array norm;
} Item;


static int orderItemsById (Item *a,Item *b)
{
  return strcmp(a->id,b->id);
}


void usagef (int level)
{
  romsg ("Description: \n"
	 "\n"
	 "Calculate normalized read counts for input GCT file. \n"
         "Source: \n"
	 "https://haroldpimentel.wordpress.com/2014/05/08/what-the-fpkm-a-review-rna-seq-expression-units/ \n"
	 "Note that NaN are output as zero 0. \n"
	 "\n"
         "Usage: %s -i GCT-file -l Length-file [-cpm|rpkm|tpm] [-log2|log10] [-col INT] [-digits INT] \n"
	 "\n"
	 "\n"
	 "Mandatory input parameters: \n"
	 "\n"
	 "\t-g     GCT file with read counts per gene (unique gene identifier in 1st column): \n"
	 "\t-l     tab-delimited file with gene identifier in 1st and gene length in \n"
	 "\t       2nd columns, respectively. \n"
         "\t       These files can be found in the corresponding genome annotation folders, \n"
	 "\t       e.g. for human in folder /<path to genomes folder>/hg38/gtf/refseq/ \n"
	 "\n"
	 "\n"
	 "Optional input parameters: \n"
	 "\n"
	 "\t-tpm     transcript per million (default) \n"
	 "\t-rpkm    reads per kilobase of exon per million reads mapped \n"
	 "\t-cpm     counts per million mapped reads \n"
         "\t-log2    log2 transform output (adding 0.01) \n"
         "\t-log10   log10 transform output (adding 0.01) \n"
         "\t-col     if input length file contains several columns, then specify \n"
	 "\t         the column number with this index (default last column) \n"
	 "\t-digits  number of digits after comma for output (default %d) \n"
	 "\n"
	 "\n"
         " Report bugs and feedback to %s \n",
         arg_getProgName (),DIGITS,AUTHOR_MAIL);
}



int main (int argc,char *argv[])
{

  if (arg_init (argc,argv,"tpm,0 rpkm,0 cpm,0 log2,0 log10,0 col,1 digits,1","g l",usagef) != argc)
    die ("wrong number of arguments; invoke program without params for help");

  Texta it;
  LineStream ls;
  Array items;
  char *line;
  Item *currItem;
  Item oneItem;
  int index,i,j,nsamples,method;
  float x;
  float y;
  items = arrayCreate (100,Item);
  Array sums;
  int missing = 0,present = 0;
  char *headerLine;
  int digits = DIGITS;

  if (arg_present ("cpm"))
    method = 1;
  else if (arg_present ("rpkm"))
    method = 2;
  else
    method = 0;

  if (arg_present ("digits"))
    digits = atoi(arg_get ("digits"));
  else
    digits = DIGITS;


  // read file with gene lengths
  ls = ls_createFromFile (arg_get ("l"));
  while (line = ls_nextLine (ls)) {
    it = textFieldtokP(line,"\t");
//    it = textStrtokP (line,"\t");
    currItem = arrayp (items,arrayMax (items),Item);
    currItem->id = hlr_strdup (textItem (it,0));
    if (arg_present ("col")) {
      index = atoi (arg_get ("col")) - 1;
      if (index < 1 || index+1 > arrayMax (it))
        die ("given column index is invalid. Abort!");
      currItem->len = atoi(textItem (it,index));
    }
    else
      currItem->len = atoi(textItem (it,arrayMax (it)-1));
    currItem->flag = 0;
    textDestroy (it);
  }
  ls_destroy (ls);
  arraySort (items,(ARRAYORDERF)orderItemsById);
  
  // read gct file with read counts
  ls = ls_createFromFile (arg_get ("g"));
  while (line = ls_nextLine (ls)) {
    if (ls_lineCountGet (ls) < 2)
      continue;
    else if (ls_lineCountGet (ls) == 2) {
      it = textFieldtokP(line,"\t");
      //it = textStrtokP (line,"\t");
      if (arrayMax (it) != 2)
        die ("Error in gct file header: head line #2 does not have 2 columns.");
      nsamples = atoi (textItem (it,1));
      textDestroy (it);
      sums = arrayCreate (nsamples,float);
      for (i=0;i<nsamples;i++)
        array (sums,arrayMax (sums),float) = 0.;
      continue;
    }
    else if (ls_lineCountGet (ls) == 3)  {
      headerLine = hlr_strdup (line);
      continue;
    }
    //it = textStrtokP (line,"\t");
    it = textFieldtokP(line,"\t");
    if (nsamples != arrayMax (it)-2)
      die ("inconsistency on line %s",line);
    oneItem.id = textItem (it,0);
    if (arrayFind (items,&oneItem,&index,(ARRAYORDERF)orderItemsById)) {
      present++;
      currItem = arrp (items,index,Item);
      currItem->flag = 1;
      currItem->desc = hlr_strdup (textItem (it,1));
      currItem->norm = arrayCreate (nsamples,float);
      for (i=2;i<arrayMax (it);i++) {        
        if (method == 1 || method == 2) // CPM or RPKM
          x = (float) atoi (textItem (it,i));
        else // TPM
          x = (float) atoi (textItem (it,i)) / currItem->len*1.e-3;
        array (currItem->norm,arrayMax (currItem->norm),float) = x;
        arru (sums,i-2,float) += x*1.e-6;
      }
    }
    else {
      missing++;
      warn ("Missing gene %s %s",oneItem.id,textItem (it,1));
    }
    textDestroy (it);
  }
  ls_destroy (ls);


  // output normalized read counts
  printf ("#1.2\n%d\t%d\n%s\n",present,nsamples,headerLine);
  for (i=0;i<arrayMax (items);i++) {
    currItem = arrp (items,i,Item);
    if (currItem->flag == 1) {
      printf ("%s\t%s",currItem->id,currItem->desc);
      for (j=0;j<nsamples;j++) {
        if (method == 1) // CPM
          x = arru (currItem->norm,j,float) / arru (sums,j,float); 
        else if (method == 2) // RPKM
          x = 1.e3 * arru (currItem->norm,j,float) / arru (sums,j,float) / currItem->len;
        else // TPM
          x = arru (currItem->norm,j,float)/arru (sums,j,float);
        if (arg_present ("log2"))
	  y = log2 (x+0.01);
        else if (arg_present ("log10"))
	  y = log10 (x+0.01);
        else
          y = x;
	if (isnan(y))
	  y = 0.;
	//printf ("\t%.3f",y);
	printf("\t%.*f", digits, y);
      }
      printf ("\n");
    }
  }
  if (missing > 0)
    warn ("%d genes present in the GCT file are missing in the gene length file.",missing);

  return 0;
}
