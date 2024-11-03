#!/bin/bash

: '
  Simple utility to compute the sha256 checksum of a downloaded file
  to aid in hermetic behaviour of builds as expected by Bazel

    e.g.

      bash ./.scripts/checksum.sh -t https://some.url.com/download

'

# Utils
usage() {
  usage_desc="Opts of [$0] {
              \t-t | --target <string> input file
              \t-o | --outdir <string> optional output directory, otherwise prints to console
              }"

  printf "$(pretty_print usage_desc)" 1>&2;
  exit 1;
}

pretty_print() {
  local i
  local out=''
  local args=( "$@" )
  for i in "${!args[@]}"; do
    if [ "$i" -gt "0" ]; then
      out+=('\n')
    fi

    local -n reference="${args[$i]}"
    while IFS= read -r line || [[ -n $line ]]; do
      out+="$(echo "$line" | sed 's/^[[:space:]]*//')"'\n'
    done <<< $reference
  done

  echo $out
}


# Parse opts
while [[ "$#" -gt 0 ]]
  do
    case $1 in
      -t|--target) target="$2"; shift;;
      -o|--output) outdir="$2"; shift;;
    esac
    shift
done


# Validate opts
if [ -z $target ]; then
  printf "No target URL specified...\n"
  usage
fi


# Download and compute checksum
dir=$(pwd)

cd /tmp
  file="$(curl -O --remote-name -s -L -w "%{filename_effective}" $target)"
  checksum=$(sha256sum "/tmp/$file" | cut -d ' ' -f 1)
rm -f /tmp/$file
cd $dir

content="Result {
          \t- Target   : $target
          \t- Filename : $file
          \t- Checksum : $checksum
        }"

if [ ! -z "$outdir" ]; then
  printf "$(pretty_print content)\n" > "$outdir/$file.checksum";
else
  printf "$(pretty_print content)"
fi
