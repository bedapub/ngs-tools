#!/bin/bash
#
# Replace the sample names in the GCT file by
# the names given in the SAMPLE file.

umask 2

# Usage info
show_help() {
  cat << EOF

  Usage: ${0##*/} [-h] -g GCT_FILE -s SAMPLE_FILE
  
  Replace the sample names in the GCT file by
  the names given in the SAMPLE file.

    -g   input GCT file
    -s   input SAMPLE file
         2 columns required: 
           1st: sample names in input GCT file
           2nd: sample names in output GCT file

  Optional arguments

    -h   display this help and exit

   Contact roland.schmucki@roche.com

EOF
}

err() {
  echo "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $*" >&2
}

if [[ $# -lt 2 ]]; then
  show_help
  exit 0
fi

# Parse input options
while getopts hg:s: opt; do
  case ${opt} in
    g) gFile=${OPTARG}
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
if [ ! -e ${sFile} ]; then
  err "Input SAMPLE File ${sFile} does not exist. Abort!"
  exit 1
fi

awk -v FS="\t" '{if (NR==FNR) {a[$1]=$2; next} \
  if (FNR==3){printf("%s\t%s", $1, $2);\
  for (i=3; i<=NF; i++) {printf("\t%s", a[$i])};\
  printf("\n")} else {print $0}}' ${sFile} ${gFile}

exit 0
