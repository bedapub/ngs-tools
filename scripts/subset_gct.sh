#!/bin/bash
#
# Creates a subset of the input GCT file by using only specific
# samples and/or features(keys) specified by input files

umask 2

# Usage info
show_help() {
  cat << EOF
  Usage: ${0##*/} [-h] -g GCT_FILE -k KEYS_FILE -s SAMPLES_FILE

  Creates a subset of the input GCT file 
  
    -g   input GCT file
    -k   input KEYS file
         1 column required: 
           1st: keys (e.g. Genes) in input GCT file
    -s   input SAMPLES file
         1 column required:
           1st: samples names in input GCT file to output
  Optional arguments

    -h   display this help and exit

   Contact roland.schmucki@roche.com
EOF
}

err() {
  echo "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $*" >&2
}

if [[ $# -lt 1 ]]; then
  err "Invalid number of input arguments. Abort!"
  show_help
  exit 0
fi

kFile=NULL
sFile=NULL

# Parse input options
while getopts hg:k:s: opt; do
  case ${opt} in
    g) gFile=${OPTARG}
      ;;
    k) kFile=${OPTARG}
      ;;
    s) sFile=${OPTARG}
      ;;
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

if [ ! -e ${gFile} ]; then
  err "Input GCT File ${gFile} does not exist. Abort!"
  exit 1
fi
if [ ${kFile} != "NULL" ] && [ ! -e ${kFile} ]; then
  err "Input KEYS File ${kFile} does not exist. Abort!"
  exit 1
fi
if [ ${sFile} != "NULL" ] && [ ! -e ${sFile} ]; then
  err "Input SAMPLES File ${sFile} does not exist. Abort!"
  exit 1
fi

# Subset by KEYS file
if [ -e ${kFile} ]; then
  awk 'BEGIN{FS="\t"}{if(NR==FNR) {a[$1]=1;next} if(a[$1]==1) \
    {print $0}}' ${kFile} ${gFile} > tmp1_$$
  n="$(wc -l tmp1_$$ | cut -d\  -f1)"
  awk 'BEGIN{printf("#1.2\n")}{if (NR==2){printf("'$n'\t%d\n",$2)} \
    else if (NR==3){print $0}}' ${gFile} > out_tmp_$$
  cat tmp1_$$ >> out_tmp_$$
  rm tmp1_$$
else
  cat ${gFile} > out_tmp_$$
fi

# Subset by SAMPLES file
if [ -e ${sFile} ]; then
  awk 'BEGIN{FS="\t"}{if (NR==3){for (i=3;i<=NF;i++) \
    {printf("%s\t%d\n",$i,i)}}}' out_tmp_$$ > tmp1_$$
  s="$(awk 'BEGIN{FS="\t"}{if(NR==FNR) {a[$1]=1;b[$1]=$2;next} \
    if(a[$1]==1) {printf(",%d",$2)}}' ${sFile} tmp1_$$)"
  n="$(wc -l ${sFile} | cut -d\  -f1)"
  cols="$(echo 1,2${s})"
  cut -f$cols out_tmp_$$ > tmp1_$$
  awk 'BEGIN{FS="\t"}{if (NR==2){printf("%d\t'$n'\n",$1)} \
    else {print $0}}' tmp1_$$
  rm tmp1_$$
else
  cat out_tmp_$$
fi

rm -f out_tmp_$$

exit 0
