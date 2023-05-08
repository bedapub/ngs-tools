#include <unistd.h>
#include "format.h"
#include "log.h"
#include "linestream.h"
#include "arg.h"
#include "rofutil.h" 

#define AUTHOR_MAIL "roland.schmucki@roche.com"

typedef struct {
  int gid;
  char *sym;
  char *chr;
  char *desc;
  char str;
  int beg;
  int end;
  int rev;
} Locus;

static int verbose = 0;
static Array loci; // of Locus

int orderLociByCoord (Locus *a,Locus *b)
{
  int r = strcmp (a->chr,b->chr);
  if (r != 0)
    return r;
  r = a->beg - b->beg;
  if (r != 0)
    return r;
  return a->end - b->end;
}

static int findLocus (int pos,char *chr)
{
  int l=0,r=arrayMax (loci)-1,x;
  int v;
  Locus *currLocus;
 
  while (r >= l) {
    x = (l+r)/2;
    currLocus = arrp (loci,x,Locus);
    v = strcmp (chr,currLocus->chr);
    if (v == 0) {
      if (pos >= currLocus->beg && pos <= currLocus->end)
        return x;
      v = pos - currLocus->beg;
    }
    if (v < 0)
      r = x-1;
    else
      l = x+1;
  }
  return -1;
}


static int findLocusLoop (int pos,char *chr)
{
  int i;
  Locus *currLocus;
 
  for (i=0;i<arrayMax (loci);i++) {
    currLocus = arrp (loci,i,Locus);
    if (strEqual (currLocus->chr, chr) &&
        pos >= currLocus->beg && 
	pos <= currLocus->end)
      return i;
  }
  return -1;
}


void print_gct (int ibeg, int iend, Texta it0)
{
  int i;
  Locus *currLocus;

  printf ("%s\t", textItem (it0,0));
  if (ibeg > -1) {
    currLocus = arrp (loci, ibeg, Locus);
    printf ("%s", currLocus->sym);
  }
  if (iend > -1 && iend != ibeg) {
    if (ibeg > -1)
      printf ("|");
      currLocus = arrp (loci, iend, Locus);
      printf ("%s", currLocus->sym);
    }
    if (ibeg < 0 && iend < 0)
      printf ("n/a");
  for (i=2; i<arrayMax (it0); i++)
    printf ("\t%s",textItem (it0,i));
  printf ("\n");
}


int print_topTable (int ibeg, int iend, char *line)
{
  Locus *currLocus;
  Locus *currLocus1;

  printf ("%s", line);

  if (ibeg < 0 && iend < 0) {
    printf ("\tn/a\tn/a\tn/a\n");
    return 0;
  }
  else if ((ibeg > -1 && iend < 0) || (ibeg == iend)) {
    currLocus = arrp (loci, ibeg, Locus);
    printf ("\t%d\t%s\t%s\n", 
            currLocus->gid, currLocus->sym, currLocus->desc);
    return 0;
  }
  else if (iend > -1 && ibeg < 0) {
    currLocus = arrp (loci, iend, Locus);
    printf ("\t%d\t%s\t%s\n",
            currLocus->gid, currLocus->sym, currLocus->desc);
    return 0;
  }
  currLocus = arrp (loci, ibeg, Locus);
  currLocus1 = arrp (loci, iend, Locus);
  printf ("\t%d|%d\t%s|%s\t%s|%s\n", 
          currLocus->gid, currLocus1->gid,
	  currLocus->sym, currLocus1->sym,
	  currLocus->desc, currLocus1->desc);
  return 0;
}


void usagef (int level)
{
  romsg ("Description: \n"
	 "\n"
         "This tool takes one input file with genomic coordinates in its first column and \n"
	 "an additional file with gene locus information. It then tries to annoate all \n"
	 "genomic regions from the first file, line by line, \n"
	 "with the annotations from the second file by overlapping the coordinates. \n"
	 "\n"
         "Usage: %s -i FILE -loci FILE -format gct|topTable \n"
	 "\n"
         "\t-i                    input file with loci information (required), ie first column \n"
	 "\t                      must contain a coordinate string \n"
	 "\t                      CHR:BEGIN-END , ie separated by colon and dash. If the input \n"
	 "\t                      format is gct or topTable then all subsequent columns sent to stdout \n"
         "\t-loci                 input file with loci information (required), tab-delimited format: \n"
	 "\t                      CHR   BEGIN   END   STRAND   GENE   SYMBOL  DESCRIPTION \n"
	 "\t-format gct|topTable  input file -i is in gct|topTable format (optional) \n"
	 "\t-verbose              show more information (optional) \n"
	 "\n"
	 "Report bugs and feedback to %s \n",
         arg_getProgName (), AUTHOR_MAIL);
}


int main (int argc,char *argv[])
{
  if (arg_init (argc, argv, "format,1 verbose,0", "i loci", usagef) != argc)
    die ("wrong number of arguments; invoke program without params for help");
 
  loci = arrayCreate (20000,Locus);
  char *chr = NULL;
  char *line;
  Texta it, it0;
  LineStream ls;
  Locus *currLocus;
  int beg;
  int end;
  int ibeg;
  int iend;
  int inputFormat;

  // set verbosity
  if (arg_present ("verbose"))
    verbose = 1;

  // input file format
  if (arg_present ("format")) {
    if (strEqual (arg_get ("format"), "gct"))
      inputFormat = 1;
    else if (strEqual (arg_get ("format"), "topTable"))
      inputFormat = 2;
    else
      die ("Invalid input format");
  }
  else {
    inputFormat = 0;
  }

  // read loci from input file
/*
  CHR     BEG        END         STR     GENE    SYMBOL  DESC
  chr19   58345183   58353492    -       1       A1BG    alpha-1-B glycoprotein
  chr12   9067708    9116229     -       2       A2M     alpha-2-macroglobulin
*/
  ls = ls_createFromFile (arg_get ("loci"));
  while (line = ls_nextLine (ls)) {
    it = textFieldtokP (line,"\t");
    if (arrayMax (it) < 7)
      die("Wrong number of fields on line: %s (max %d)", line, arrayMax (it));
    currLocus = arrayp (loci, arrayMax (loci), Locus);
    currLocus->chr = hlr_strdup (textItem (it,0));
    currLocus->beg = atoi (textItem (it,1));
    currLocus->end = atoi (textItem (it,2));
    currLocus->str = textItem (it,3)[0];
    currLocus->gid = atoi (textItem (it,4));
    currLocus->sym = hlr_strdup (textItem (it,5));
    currLocus->desc = hlr_strdup (textItem (it,6));
    textDestroy (it);
  }    
  ls_destroy (ls);
  arraySort (loci,(ARRAYORDERF)orderLociByCoord);
  /*for (i=0;i<arrayMax (loci);i++) {
    currLocus = arrp (loci,i,Locus);
    printf ("# sorted by coordinates\t%s\t%d\t%s\t%d\t%d\t%c\t%s\n",
            currLocus->sym,currLocus->gid,currLocus->chr,
            currLocus->beg,currLocus->end,currLocus->str,
            currLocus->desc);
  }*/

  // parse input file and annotate
  ls = ls_createFromFile (arg_get ("i"));
  while (line = ls_nextLine (ls)) {
    if (inputFormat == 1 && ls_lineCountGet (ls) < 4) { // GCT format
      puts (line);
      continue;
    }
    if (inputFormat == 2 && ls_lineCountGet (ls) < 2) { // TopTable format
      printf ("%s\tGENE\tSYMBOL\tDESCRIPTION\n",line);
      continue;
    }

    it0 = textStrtokP (line,"\t");
    if (inputFormat == 2)
      it = textStrtokP (textItem (it0,1),":-");
    else
      it = textStrtokP (textItem (it0,0),":-");
    if (arrayMax (it) < 3) {
      textDestroy (it);
      textDestroy (it0);
      romsg ("Skip line: %s", line);
      continue;
    }
    strReplace (&chr, textItem (it,0));
    beg = atoi (textItem (it,1));
    end = atoi (textItem (it,2));
    ibeg = findLocus (beg,chr);
    iend = findLocus (end,chr);

    if (ibeg == -1) {
      ibeg = findLocusLoop (beg,chr);
      if (verbose == 1 && ibeg > -1)
        romsg ("# applied loop to find locus: chr=%s\tbeg=%d\tend=%d\t-->\tibeg=%d", chr, beg, end, ibeg);
    }
    if (iend == -1) {
      iend = findLocusLoop (end,chr);
      if (verbose == 1 && iend > -1)
        romsg ("# applied loop to find locus: chr=%s\tbeg=%d\tend=%d\t-->\tiend=%d", chr, beg, end, iend);
    }

    if (inputFormat == 1)
      print_gct (ibeg, iend, it0);
    else if (inputFormat == 2)
      print_topTable (ibeg, iend, line);
    else {
      printf ("%s\t", textItem (it0,0));
      if (ibeg > -1) {
        currLocus = arrp (loci, ibeg, Locus);
        printf ("%s", currLocus->sym);
      }
      if (iend > -1 && iend != ibeg) {
        if (ibeg > -1)
          printf ("|");
        currLocus = arrp (loci, iend, Locus);
        printf ("%s", currLocus->sym);
      }
      if (ibeg < 0 && iend < 0)
        printf ("n/a");
      printf ("\n");
    }
    textDestroy (it);
    textDestroy (it0);
  }
  ls_destroy (ls);

  return 0;
}
