#!/bin/bash
#
# Creates a documentation in md format for all 
# C programs and scripts

umask 2

# Usage info
show_help() {
  cat << EOF
  Usage: ${0##*/} [-h] -b DIR -o FILE

  Create documentation MD files for all tools.
  
    -b   input bin directory containing all executables
    -o   output file name

  Optional arguments

    -h   display this help and exit

   Contact roland.schmucki@roche.com
EOF
}

err() {
  echo "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $*" >&2
}

if [[ $# -ne 4 ]]; then
  err "Invalid number of input arguments. Abort!"
  show_help
  exit 0
fi

outfile=NULL
bindir=NULL

# Parse input options
while getopts ho:b: opt; do
  case ${opt} in
    o) outfile=${OPTARG}
      ;;
    b) bindir=${OPTARG}
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

if [ ! -d ${bindir} ]; then
  err "Input bin directory ${bindir} does not exist. Abort!"
  exit 1
fi

# Make title/header
printf "# Tools\n\n" > ${outfile}

# Loop through all files in bin directory
for f in "${bindir}"/*; do
  if [ -f "$f" ]; then
    echo "$f"
    s="$(basename $f)"
    printf "## $s\n\n" >> ${outfile}
    printf "\`\`\`\n" >> ${outfile}
    ($f -h ) &>> ${outfile}
    printf "\`\`\`\n\n" >> ${outfile}
  fi
done

exit 0
