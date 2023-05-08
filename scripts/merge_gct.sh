#!/bin/bash
#
# Merge several input GCT files into one output GCT file (sent to stdout).

umask 2

# Usage info
show_help() {
  cat << EOF

  Usage: ${0##*/} [-h] FILE1 FILE2 [FILE3 ...]
  
  Merge GCT files

  Optional arguments

    -h   display this help and exit

   Contact roland.schmucki@roche.com

EOF
}

if [[ $# -lt 2 ]]; then
  show_help
  exit 0
fi

# Parse input options
while getopts h opt; do
  case ${opt} in
    h)
      show_help
      exit 0
      ;;
    \?)
      echo "Invalid option: -${OPTARG}" >&2
      show_help
      exit 1
      ;;
    :)
      echo "Option -${OPTARG} requires an argument." >&2
      show_help
      exit 1
      ;;
    *)
      show_help >& 2
      exit 1
      ;;
  esac
done
shift "$((OPTIND-1))"


# loop over all input files
for i in $@; do
  awk 'BEGIN{FS="\t"}{if (NR>3){printf("%s\n",$1)}}' $i >> keys.tmp$$
  awk 'BEGIN{FS="\t"}{if (NR>3){printf("%s\t%s\n",$1,$2)}}' $i >> desc.tmp$$
  awk 'BEGIN{FS="\t"}{if (NR==3){for (i=3; i<=NF; i++) {printf("%s\n",$i)}}}' $i \
    >> samples.tmp$$
done

sort keys.tmp$$ | uniq > keys.uniq.tmp$$ && rm keys.tmp$$
m="$(wc -l keys.uniq.tmp$$ | cut -d\  -f1)"
n="$(wc -l samples.tmp$$ | cut -d\  -f1)"

awk 'BEGIN{FS="\t"}{if(NR==FNR) {a[$1]=1; b[$1]=$2; next} if(a[$1]==1) \
  {printf("%s\n",b[$1])} else {printf("NA\n")}}' \
  desc.tmp$$ keys.uniq.tmp$$ > desc.uniq.tmp$$
  
files="$(echo keys.uniq.tmp$$ desc.uniq.tmp$$)"
j=0
for i in $@; do
  j=$((j + 1))
  files="$(echo ${files} $j.tmp$$)"
  k="$(awk 'BEGIN{FS="\t"}{if (NR==2){print $2}}' $i)"
  
  awk 'BEGIN{FS="\t"}{if(NR==FNR) {a[$1]=1; b[$1]=$0; next} if(a[$1]==1) \
    {printf("%s\n",b[$1])} else {printf("NAME\tDESC");\
    for (i=1; i<='$k'; i++) {printf("\t0")}; printf("\n")}}' \
    $i keys.uniq.tmp$$ \
    | cut -f3- > $j.tmp$$
done

awk 'BEGIN{FS="\t"; printf("#1.2\n'$m'\t'$n'\nNAME\tDESCRIPTION")}\
  {printf("\t%s",$1)} END{printf("\n")}' samples.tmp$$ 
paste ${files}

rm ${files} samples.tmp$$ desc.tmp$$

exit 0
