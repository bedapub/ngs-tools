#include "format.h"
#include "log.h"
#include "linestream.h"
#include "arg.h"
#include "rofutil.h"

#define AUTHOR_MAIL "roland.schmucki@roche.com"

static int verbose = 0;

void usagef (int level)
{
  romsg ("Description: \n"
         "\n"
	 "Parse a GTF file and output attributes to stdout. \n"
         "\n"
	  "Usage: %s [-h] -gtf INFILE  + 1 Optional argument from below \n"
         "\n"
         "Optional arguments\n"
         "\n"
         "\t-output-fields=\"gene_name,gene_synonym,product\" \n"
         "\n"
         "The above example will output all gtf fields named \"gene_name\", \"gene_synonym\", and \"product\" \n"
         "\n"
         "\t-refseq \n"
         "\n"
         "This option will work on a gtf from refseq and do the following  \n"
         "             0) discard lines that are not exons \n"
         "             1) get gene number from the field Dbxref \"GeneID: \n"
         "                and replace the input gene_id value with the gene number \n"
         "             2) remove transcript_id fields if they contain \"rnaN\" numbers where \n"
         "                N is an integer \n"
         "             3) remove version number from transcript_id field \n"
         "                (e.g. transcript_id \"NR_046018.2\" --> transcript_id \"NR_046018\") \n"
         "             4) remove duplicated transcript_id fields \n"
	 "\n"
         "Note there will be a warning if there is no proper transcript accession id (mostly miRNAs) \n"
         "\n"
         "\n"
	  "Report bugs and feedback to %s"
         "\n",
         arg_getProgName (),AUTHOR_MAIL);
}

/*
  0) filter away non-exon lines

  1) get gene number from the field Dbxref "GeneID:
  and replace the input gene_id value with the gene number
  
  2) remove transcript_id fields if they contain "rnaN" numbers where
  N is an integer

  3) remove version number from transcript_id field 
  (e.g. transcript_id "NR_046018.2" --> transcript_id "NR_046018")

  4) remove duplicated transcript_id fields

  5) output only features:
    gene_id
    gene_name
    transcript_id
*/
void parse_refseq ()
{
  int i,j;
  LineStream ls;
  char *line,*pos;
  Texta it;
  Texta fields;
  char *gene_id = NULL;
  char *gene_symbol = NULL;
  char *transcript_id = NULL;
  char *product = NULL;
  Stringa str = stringCreate (100);

  ls = ls_createFromFile (arg_get ("gtf"));
  while (line = ls_nextLine (ls)) {
    if (line[0] == '#') {
      warn ("skip line %s",line);
      continue;
    }
    it = textFieldtokP (line,"\t");
    if (arrayMax (it) < 9)
      die ("missing fields on line %s",line);

    if (!strEqual (textItem (it,2),"exon")) {
      warn ("no exon on line %s",line);
      textDestroy (it);
      continue;
    }
    fields = textStrtokP (textItem (it,8),";");


    // get gene_id
    stringClear (str);
    pos = strstr (textItem (it,8),"GeneID:");
    if (pos != NULL) {
      for (i=7;i<strlen (pos);i++) {
        if (pos[i] == ',' || pos[i] == '"')
          break;
        stringCatChar (str,pos[i]);
      }
    }
    if (stringLen (str) == 0) {
      //warn ("GeneID not valid for line %s",line);
      gene_id = NULL;
    }
    else
      strReplace (&gene_id,string (str));


    // get transcript_id
    stringClear (str);
    for (i=0;i<arrayMax (fields);i++) {
      if (strStartsWith (textItem (fields,i)," transcript_id \"") &&
          !strStartsWith (textItem (fields,i)," transcript_id \"rna")) {
        for (j=16;j<strlen (textItem (fields,i));j++) {
          if (textItem (fields,i)[j] == '.')
            break;
          stringCatChar(str,textItem (fields,i)[j]);
        }
        break;
      }
    }
    if (stringLen (str) == 0) {
      stringClear (str);
      //warn ("transcript_id not valid for line %s",line);
      /* use internal transcript_id */
      pos = strstr (textItem (it,8),"transcript_id \"");
      if (pos != NULL) {
        for (i=15;i<strlen (pos);i++) {
          if (pos[i] == '"')
            break;
          stringCatChar (str,pos[i]);
        }
      }
      strReplace (&transcript_id,string (str));
    }
    else
      strReplace (&transcript_id,string (str));
    

    // get gene name (symbol)
    stringClear (str);
    pos = strstr (textItem (it,8),"gene_name \"");
    if (pos != NULL) {
      for (i=11;i<strlen (pos);i++) {
        if (pos[i] == '"')
          break;
        stringCatChar (str,pos[i]);
      }
    }
    if (stringLen (str) == 0) {
      //warn ("gene symbol not valid for line %s",line);
      gene_symbol = NULL;
    }
    else
      strReplace (&gene_symbol,string (str));

    // get product ?
    stringClear (str);
    pos = strstr (textItem (it,8),"product \"");
    if (pos != NULL) {
      for (i=9;i<strlen (pos);i++) {
        if (pos[i] == '"')
          break;
        stringCatChar (str,pos[i]);
      }
    }
    if (stringLen (str) == 0) {
      //warn ("product not valid for line %s",line);
      product = NULL;
    }
    else
      strReplace (&product,string (str));

    /* output */
    printf ("%s",textItem (it,0));
    for (i=1;i<8;i++)
      printf ("\t%s",textItem (it,i));
    printf ("\tgene_id \"%s\";",gene_id);
    printf (" gene_symbol \"%s\";",gene_symbol);
    printf (" transcript_id \"%s\";",transcript_id);
    if (product)
      printf (" product \"%s\";",product);
    printf ("\n");

    /* destroy arrays */
    textDestroy (fields);
    textDestroy (it);
  }
  ls_destroy (ls);
}


int main (int argc,char *argv[])
{

  if (arg_init (argc,argv,"verbose,0 output-fields,1 refseq,0","gtf",
                usagef) != argc)
    die ("wrong number of arguments; invoke program without params for help");
  
  if (arg_present ("verbose"))
    verbose = 1;


  if (arg_present ("refseq")) {
    parse_refseq ();
    return 0;
  }

  /* output fields */
  int i,j,k,n;
  LineStream ls;
  char *line;
  Texta it;
  char *pos;
  Texta fields;
  Stringa str = stringCreate (100);
  
  fields = textFieldtokP (arg_get ("output-fields"),",");
  ls = ls_createFromFile (arg_get ("gtf"));
  while (line = ls_nextLine (ls)) {
    if (line[0] == '#') {
      warn ("skip line %s",line);
      continue;
    }
    it = textFieldtokP (line,"\t");
    /*      n=1;
            printf ("%s",textItem (it,0));
            for (i=1;i<8;i++) 
            printf ("\t%s",textItem (it,i));*/
    n = 0;
    for (i=0;i<arrayMax (fields);i++) {
      pos = strstr (textItem (it,8),textItem (fields,i));
      if (pos != NULL) {
        stringClear (str);
        k = 0;
        for (j=0;j<(int)strlen (pos);j++) {
          if (k == 2)
            break;
          else if (pos[j] == '"')
            k++;
          else if (k == 1)
            stringCatChar (str,pos[j]);
        }
        if (n > 0)
          printf ("\t");
        printf ("%s",string (str));
        n++;
      }
    }
    if (n > 0)
      printf ("\n");
    textDestroy (it);
  }
  ls_destroy (ls);

  return 0;
}
