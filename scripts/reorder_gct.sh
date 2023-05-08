#!/bin/bash
#
# Re-order samples (columns) in the GCT file by
# the names given in the SAMPLE file.

umask 2

# Usage info
show_help() {
  cat << EOF

  Usage: ${0##*/} [-h] -g GCT_FILE -s SAMPLE_FILE
  
  Re-order samples (columns) in the GCT file by
  the names given in the SAMPLE file.

    -g   input GCT file
    -s   input SAMPLE file with re-ordered sample names (file must contain exactly one column)

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
  err "Input GCT File $gFile does not exist. Abort!"
  exit 1
fi
if [ ! -e ${sFile} ]; then
  err "Input SAMPLE File $sFile does not exist. Abort!"
  exit 1
fi

# From Nikos:
awk -v FS="\t" '{if(NR==FNR){a[$i]=NR+2; next} s=$1"\t"$2; \
  if(FNR==3){for(i=3 ;i<=NF; i++)b[$i]=i; for(x in a) c[a[x]]=b[x]} \
  if(FNR>=3)for(i=3; i<=NF; i++){s=s"\t"$c[i]} print s}' \
  ${sFile} ${gFile}

exit 0
