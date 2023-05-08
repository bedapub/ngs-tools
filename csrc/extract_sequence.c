#include "format.h"
#include "log.h"
#include "linestream.h"
#include "arg.h"

#define AUTHOR_MAIL "roland.schmucki@roche.com"

int verbose = 0;

void usagef (int level)
{
  romsg ("Description: \n"
         "\n"
         "Extract from an input fasta or fastq file sequences by ids from another input file.\n"
	 "\n"
         "Usage: %s [-verbose] [-delimiter='. TAB'] [-useEntireIdLine] \n"
         "          [-quick] [-not] -ids ids_file -fasta|fastq fasta_file \n"
	 "\n" 
	 "Mandatory parameters: \n"
	 "\n"
	 "\t  -ids              file name containing sequence id's \n" 
	 "\t  -fasta|fastq      file name containing the fasta or fastq sequences \n"
	 "\n"
	 "Optional parameters: \n"
	 "\n"
	 "\t  -delimiter        delimiter on the sequence id line \n"
	 "\t  -useEntireIdLine  use the entire line as id and not split line by \n"
	 "\t                    -delimiter into fields \n"
	 "\t  -quick            stop search after the first match \n"
	 "\t  -not              inverse the search, ie output sequences that are \n"
	 "\t                    not in the ids file \n"
	 "\t  -verbose          output additional information \n"
	 "\n"
	 "\n"
         "Report bugs and feedback to %s \n",
         arg_getProgName (), AUTHOR_MAIL);
}


int main (int argc,char *argv[])
{
  char *line = NULL;
  LineStream ls;
  Texta ids = arrayCreate (1000,Texta);
  Texta it;
  int not = 0;
  int quick = 0;
  Stringa delim = NULL;
  Stringa str = stringCreate (100);

  if (arg_init (argc,argv,"verbose,0 delimiter,1 useEntireIdLine,0 quick,0 not,0 fasta,1 fastq,1","ids",usagef) != argc)
    die ("wrong number of arguments; invoke program without params for help");
  
  if (arg_present ("verbose"))
    verbose = 1;
  if (arg_present ("not"))
    not = 1;
  if (arg_present ("quick"))
    quick = 1;

  if (arg_present ("delimiter")) {
    delim = stringCreate (10);
    if (strstr (arg_get ("delimiter"),"TAB")) {
      stringClear (delim);
      char *tmp = hlr_strdup (arg_get ("delimiter"));
      int i;
      for (i=0;i<strlen (tmp);i++) {
        if (tmp[i] != 'T' && tmp[i] != 'A' && tmp[i] != 'B') {
          stringCatChar (delim,tmp[i]);
        }
      }
      hlr_free (tmp);
      stringCat (delim,"\t");
    }
    else
      stringPrintf (delim,"%s",arg_get ("delimiter"));
    if (verbose)
      printf ("#INFO field delimiter \"%s\"\n",string (delim));
  }

  /* input read identifiers from ids file */
  ls = ls_createFromFile (arg_get ("ids"));
  while (line = ls_nextLine (ls)) {
    if (strstr (line,"Ensembl"))
      continue;
    if (delim != NULL)
      it = textFieldtokP (line,string (delim));
    else
      it = textFieldtokP (line," \t");
    if (arg_present ("useEntireIdLine")) 
      textAdd (ids,line);
    else if (textItem (it,0)[0]=='>')
      textAdd (ids,textItem (it,0)+1);
    else
      textAdd (ids,textItem (it,0));
    textDestroy (it);
  }
  ls_destroy (ls);
  arraySort (ids,(ARRAYORDERF)arrayStrcmp);


  char *id = NULL;
  int index;
  int found = 0;
  int doPrint;
  int k = 0;
  char *key = NULL;
  /*
    @RBAMWPRLAB1051:169:C250MACXX:1:1101:1179:2185 1:N:0:ACAGTG
    CTTAAGTACATTGAAACCCTTAATGTTCCTGGAGCTGTGTTGGTTTTTTTG
    +
    CCCFFFDFHHHHHJJJJJJJJJJJJHJJIJJJGHJIIFHHIJIBHHIJJJJ
  */
  if (arg_present ("fastq")) {
    if (strstr (arg_get ("fastq"),".gz")) {
      stringPrintf (str,"gunzip -c %s",arg_get ("fastq"));
      ls = ls_createFromPipe (string (str));
    }
    else
      ls = ls_createFromFile (arg_get ("fastq"));
    while (line = ls_nextLine (ls)) {
      /* fetch read name key from first line */
      if (key == NULL) {
        it = textFieldtokP (line,":");
        strReplace (&key,textItem (it,0));
        textDestroy (it);
      }
      if (doPrint == 1 && !strStartsWith (line,key))
        puts (line);
      if (!strStartsWith (line,key))
        continue;

      if (quick == 1 && found == arrayMax (ids))
	break;
      
      doPrint = 0;

      /* use read id up to first space */
      it = textFieldtokP (line," ");
      if (line[0] == '@')
        id = textItem (it,0)+1;
      else
        id = textItem (it,0);
      k = arrayFind (ids,&id,&index,(ARRAYORDERF)arrayStrcmp);
      if (verbose) {
        if (k==1)
          printf ("Found %s\tk=%d\t%s\n",id,index,textItem (ids,index));
        else 
          printf ("Notfound. %s\n",line);
      }
      textDestroy (it);
      if ((k && not == 0)||(!k && not == 1)) {
        found++;
        doPrint = 1;
        puts (line);
      }
    }
    ls_destroy (ls);
  }
  else {
    if (strstr (arg_get ("fasta"),".gz")) {
      stringPrintf (str,"gunzip -c %s",arg_get ("fasta"));
      ls = ls_createFromPipe (string (str));
    }
    else
      ls = ls_createFromFile (arg_get ("fasta"));
    while (line = ls_nextLine (ls)) {
      if (doPrint == 1 && line[0] != '>')
        puts (line);
      if (line[0] != '>')
        continue;
      
      if (quick == 1 && found == arrayMax (ids))
	break;
      
      doPrint = 0;
      
      if (delim != NULL)
        //        it = textFieldtokP (line,"\t.");
        it = textFieldtokP (line,string (delim));
      else
        it = textFieldtokP (line," \t");
      
      if (arg_present ("useEntireIdLine")) 
        id = line;
      else
        strReplace (&id,textItem (it,0)+1);
      
      k = arrayFind (ids,&id,&index,(ARRAYORDERF)arrayStrcmp);
      
      if (verbose) {
        if (k==1)
          printf ("Found %s\tk=%d\t%s\n",id,index,textItem (ids,index));
        else {
          printf ("Notfound. %d fields,field 1=\"%s\"\n",arrayMax (it),id);
      }
      }
      
      if ((k && not == 0)||(!k && not == 1)) {
        found++;
        doPrint = 1;
        puts (line);
      }
    textDestroy (it);
    }
    ls_destroy (ls);

    //    if (not == 0 && found != arrayMax (ids))
    //      printf ("# not found ids %d\n",arrayMax (ids)-found);
  }

  textDestroy (ids);
  return 0;
}
