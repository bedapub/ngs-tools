#!/bin/bash
#
# Filter away all features from a GCT file if the row MIN or MAX 
# is lower/greater/lower equal (MIN-EQUAL)/greater equal (MAX-EQUAL)
# than a user given threshold. Use MIN-/MAX-REVERSE to output reversed comparison.
# Results are redirected to the standard output.

umask 2

# Default values for optional arguments:

if [[ $# -ne 3 ]]; then
  echo ""
  echo "  Filter away all features from a GCT file if the row "
  echo "  MIN or MAX is lower/greater/lower equal (MIN-EQUAL)/greater equal (MAX-EQUAL)"
  echo "  than a user given threshold. Use MIN-/MAX-REVERSE to output reversed comparison."
  echo "  Results are redirected to the standard output."
  echo ""
  echo "  3 input arguments required:"
  echo ""
  echo "    1. input GCT file"
  echo "    2. threshold value (real number)"
  echo "    3. MIN or MAX or MIN-EQUAL or MAX-EQUAL or MIN-REVERSE or MAX-REVERSE"
  echo ""
  echo "  Contact roland.schmucki@roche.com"
  echo ""
  exit 0
fi

err() {
  echo "[$(date +'%Y-%m-%dT%H:%M:%S%z')]: $*" >&2
}

gct=$1
if [ ! -e ${gct} ]; then
  err "Input GCT file ${gct} does not exist. Abort!"
  exit
fi

thr=$2
tmp="$(mktemp)"

if [[ $3 == 'MIN' ]]; then
  awk 'BEGIN{FS="\t"}{if (NR>3){min=$3;max=$3;for (i=4;i<=NF;i++){if ($i>max){max=$i}; if ($i<min){min=$i}};if (min>'${thr}'){print $0}}}' ${gct} > ${tmp}
elif [[ $3 == 'MIN-EQUAL' ]]; then
  awk 'BEGIN{FS="\t"}{if (NR>3){min=$3;max=$3;for (i=4;i<=NF;i++){if ($i>max){max=$i}; if ($i<min){min=$i}};if (min>='${thr}'){print $0}}}' ${gct} > ${tmp}
elif [[ $3 == 'MIN-REVERSE' ]]; then
  awk 'BEGIN{FS="\t"}{if (NR>3){min=$3;max=$3;for (i=4;i<=NF;i++){if ($i>max){max=$i}; if ($i<min){min=$i}};if (min<'${thr}'){print $0}}}' ${gct} > ${tmp}
elif [[ $3 == 'MAX' ]]; then
  awk 'BEGIN{FS="\t"}{if (NR>3){min=$3;max=$3;for (i=4;i<=NF;i++){if ($i>max){max=$i}; if ($i<min){min=$i}};if (max<'${thr}'){print $0}}}' ${gct} > ${tmp}
elif [[ $3 == 'MAX-EQUAL' ]]; then
  awk 'BEGIN{FS="\t"}{if (NR>3){min=$3;max=$3;for (i=4;i<=NF;i++){if ($i>max){max=$i}; if ($i<min){min=$i}};if (max<='${thr}'){print $0}}}' ${gct} > ${tmp}
elif [[ $3 == 'MAX-REVERSE' ]]; then
  awk 'BEGIN{FS="\t"}{if (NR>3){min=$3;max=$3;for (i=4;i<=NF;i++){if ($i>max){max=$i}; if ($i<min){min=$i}};if (max>'${thr}'){print $0}}}' ${gct} > ${tmp}
else
  err "ERROR: argument 3 $3 is not allowed. Abort!"
  exit 1
fi
m="$(awk 'END{print NR}' ${tmp})"
head -3 ${gct} | awk 'BEGIN{FS="\t"}{if (NR==2){printf("'$m'\t%d\n",$2)}else{print $0}}'
cat ${tmp}
rm ${tmp}
exit 0
