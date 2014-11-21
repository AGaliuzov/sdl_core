#! /usr/bin/env bash

shopt -s extglob
shopt -s dotglob

srcdir=$1
bindir=$2
customer=$3
args=("$@")
#All args after customer will be passed to post_install function
additional_args=${args[@]:2}



function post_install {
# this function should be overrided by customer configuration file if nesessary
    echo "Code preparing is finished"
}

function find_in_excludes_src() {
	upper=${#excludes_src[@]}
	lower=0
	text=$1
	while [ $lower -le $upper ]
	do
		pivot=$((($upper+$lower)/2))
		cur=${excludes_src[$pivot]}
		if [[ $text < $cur ]]
		then
			upper=$(($pivot-1))
		elif [[ $text > $cur ]]
		then
			lower=$(($pivot+1))
		else
			result=$pivot
			return 0
		fi
	done	
	result="error"
	return 1
}


function find_in_excludes_bin() {
	upper=${#excludes_bin[@]}
	lower=0
	text=$1
	while [ $lower -le $upper ]
	do
		pivot=$((($upper+$lower)/2))
		cur=${excludes_bin[$pivot]}
		if [[ $text < $cur ]]
		then
			upper=$(($pivot-1))
		elif [[ $text > $cur ]]
		then
			lower=$(($pivot+1))
		else
			result=$pivot
			return 0
		fi
	done	
	result="error"
	return 1
}

function is_excluded_src() {
  find_in_excludes_src $1
  if [[ $result != "error" ]]
  then 
	return 0
  fi

  for entry in $exclude_filenames; do
    if [[ ${1##*/} == $entry ]]; then 
      return 0	  
    fi
  done
  return 1
}

function is_excluded_bin() {
  find_in_excludes_bin $1
  if [[ $result != "error" ]]
  then 
	return 0
  fi
  
  for entry in $exclude_filenames; do
    if [[ ${1##*/} == $entry ]]; then 
      return 0	  
    fi
  done
  return 1
}

function is_to_filter() {
  for entry in @($files_to_filter); do
    if [[ ${1%%*/} == ${entry} ]]; then
      return 0
    fi
  done
  return 1
}

testme="*.sh \
        PrepareCodeForCustomer"

if [ -z $srcdir ] ||  [ -z $customer ]; then
  echo "Usage: $0 <source dir> <build dir> <customer name>" >&2
  exit 1
fi

source $srcdir/customer-specific/$customer.conf
specificdir=$srcdir/customer-specific/$customer

readarray -t excludes_src < <(for entry in $exclude_src; do echo $entry; done | sort)
readarray -t excludes_bin < <(for entry in $exclude_bin; do echo $entry; done | sort)

if [ -a $export_dir ]; then
  echo "$export_dir already exists" > /dev/stderr
  exit 1
fi

mkdir $export_dir

function integrate_src() {
  relfn=${1##$2/}
  if is_excluded_src $relfn; then 
    echo $relfn" excluded"
    return
  else 
    echo "$relfn"      
  fi
  if [ -d $1 ]; then
    mkdir -p $export_dir/$relfn
    for l in $1/*; do
      integrate_src $l $2;
    done
  elif [ -f $1 ]; then
    if is_to_filter $relfn; then
      $srcdir/$filter_command $1  > $export_dir/$relfn
    else
      cp $1 $export_dir/$relfn
      echo $1" copied to "$export_dir/$relfn
    fi
  fi
}

function integrate_bin() {
  relfn=${1##$2/}
  if is_excluded_bin $relfn; then 
    return
  fi
  echo "$relfn"
  if [ -d $1 ]; then
    mkdir -p $export_dir/$relfn
    for l in $1/*; do
      integrate_bin $l $2;
    done
  elif [ -f $1 ]; then
    if is_to_filter $relfn; then
      $srcdir/$filter_command $1  > $export_dir/$relfn
    else
      cp $1 $export_dir/$relfn
    fi
  fi
}

set -f
for entry in $include_src; do
  set +f
  for p in $srcdir/$entry; do
    integrate_src $p $srcdir
  done
done

set -f
for entry in $include_bin; do
  set +f
  for p in $bindir/$entry; do
    integrate_bin $p $bindir
  done
done

set +f
cp -r $specificdir/* $export_dir/
post_install $additional_args

