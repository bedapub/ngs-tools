#!/bin/bash
#
# Sorts input GCT file by column 1 (default) or 2 in numeric or alphabetic (default) order

umask 2

# Usage info
show_help() {
  cat << EOF

  Usage: ${0##*/} [-h] -g GCT_FILE [-c 1|2] [-n] [-r]

  Sorts input GCT file by column 1 (default) or 2 in numeric or alphabetic (default) order
  

    -g   input GCT file
    -c   column 1 or 2 (default is 1)
    -n   order numerically or alphabetically (default) 
    -r   reverse order

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

gFile=NULL
col=1
num=0
rev=0

# Parse input options
while getopts hg:c:nr opt; do
  case ${opt} in
    g) gFile=${OPTARG}
      ;;
    c) col=${OPTARG}
      ;;
    n) num=1
      ;;
    r) rev=1
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
if [[ ${col} -gt 2 ]]; then
  err "Input parameter c for column must be 1 or 2. Abort!"
  exit 1
fi


# Output header
head -3 ${gFile}

# Options
opt=''
if [[ ${num} -eq 1 ]]; then
  opt="$(echo "${opt} -n")"
fi
if [[ ${rev} -eq 1 ]]; then
  opt="$(echo "${opt} -r")"
fi

# Ordered output
awk 'BEGIN{FS="\t"}{if (NR>3) {print $0}}' ${gFile} | sort -k${col} ${opt}

exit 0
