# Basic utilities bash library.

darwin=false;
case "`uname`" in
  Darwin*) darwin=true ;;
esac

if $darwin; then
  sedi="sed -i ''"
  CPUCOUNT=$(sysctl -n hw.ncpu)
else
  sedi="sed -i "
  CPUCOUNT=$(grep -c "^processor" /proc/cpuinfo)
fi

echo "$CPUCOUNT logical cores"

print_separator() {
 str=$1
 num=$2
 v=$(printf "%-${num}s" "$str")
 echo "${v// /$str}"
}

function git_repo() {
  repo=$1
  dir=$2
  commit=$3

  print_separator "=" 80
  echo  Updating $dir from repo $repo
  print_separator "=" 80

  if [ ! -d $dir ]
  then
    git clone $repo $dir
  fi

  cd $dir

  if [ ! -z "$commit" ]
  then
    git reset --hard $commit
  else
    git pull
  fi

  cd ..
}

function get_archive() {
  url=$1
  filename=$2
  sha=$3

  print_separator "=" 80
  echo "  Downloading $filename from $url"
  print_separator "=" 80

  [ ! -f $2 ] && curl -L $1 -o $2
  filesha=$(shasum -a 256 $filename | cut -d' ' -f1)
  if [ $filesha != $sha ]; then
    echo "Error: Wrong hash [$filesha] Expected [$sha]"
    exit 1
  else
    if [ ${filename: -7} == ".tar.gz" ] || [ ${filename: -7} == ".tar.xz" ]; then
      print_separator "=" 80
      echo "  Untarring $filename"
      print_separator "=" 80
      tar -xf $filename
    elif [ ${filename: -4} == ".zip" ]; then
      print_separator "=" 80
      echo "  Unzipping $filename"
      print_separator "=" 80
      unzip -o $filename
    fi
  fi;
}
