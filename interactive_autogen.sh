#! /bin/bash

# Crash if any sub command crashes
set -o errexit

# Crash if any unset variables are used
set -o nounset


# Get the directory that autogen.sh is in (stolen frome stackoverflow:
# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-what-directory-its-stored-in
# ), this is the oomph-lib root directory. Doesn't follow symlinks to the
# script itself, should be robust for anything else. If you move autogen.sh
# this will need to change a little.
oomph_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd "$oomph_root"

# Load helper functions
source bin/autogen_helpers.sh


# Echo autogen's usage if requested
while getopts ":h" opt; do
  case $opt in
      h)
          echo "This script is an interactive interface to autogen.sh." 
          echo "Arguments allowed for the underlying autogen.sh are:"
          echo
          EchoUsage
          exit 0
          ;; 
  esac
done


#====================================================================
# Start Q/A session
#====================================================================

echo " "
echo "============================================================= "
echo "        oomph-lib interactive installation script" 
echo "============================================================= "
echo " "


# Choose build directory (for lib,include)
build_dir="$oomph_root/build"

echo " "
echo " "
echo "I'm going to install the distribution (the lib and include directories)"
echo "in:"
echo " "
echo "    " $build_dir
echo " "
echo " "
if ! YesNoRead "Is this OK?" "y"; then
    printf "Specify build directory [e.g. /home/joe_user/build] :"
    build_dir=$(OptionRead)
fi

echo " "
echo "============================================================= "
echo " "
echo "Build directory is: " 
echo " " 
echo "     " $build_dir
echo " " 
echo "--> The include directory will be in: "
echo " " 
echo "    " $build_dir"/include" 
echo " " 
echo "--> The lib directory will be in: "
echo " " 
echo "    " $build_dir"/lib" 
echo " "
echo "etc.       " 
echo " "
echo "============================================================= "
echo " "


# Choose configure options file
#------------------------------

# Ask if the initial options are OK
echo " "
echo "Configure options are: "
cat "config/configure_options/current"
echo 
if YesNoRead "Is this OK?" "y"; then
    accept_configure_options="true"
else
    accept_configure_options="false"
fi

# Continue asking if the options are OK until approved
while [[ $accept_configure_options != "true" ]]; do

    # Get list of options files
    configure_option_files="$(find config/configure_options -type f)"

    echo " "
    echo "======================================================================"
    echo 
    echo "Choose an alternative configuration file "
    # Loop over files and display a menu
    count=0
    for file in $configure_option_files
    do
        #Increase the counter
        count=$(expr $count + 1)
        echo $count ": " $(basename $file)
    done

    echo
    echo "Enter the Desired configuration file [1-"$count"]"
    echo "Enter 0 to specify the options on the command line"

    # Read in the Desired File and validate it
    file_number=$(OptionRead)
    if (( $file_number >= $count )) || (( $file_number < 0 )); then
        # Error and go to start of loop
        echo "File number out of range, trying again." 1>&2
        continue
    fi


    # If options are to be read from the command line then store the
    # options in the file config/configure_options/current
    if [[ "$file_number" == "0" ]]; then
        echo 
        echo "Enter options"
        configure_options=$(OptionRead)
        echo $configure_options > "config/configure_options/current"

    # Otherwise copy the desired options file to config/configure_options/current
    else 
        # Use cut to extract the nth entry in the list
        configure_options_file="$(echo $configure_option_files | cut -d \  -f $file_number)"

        # Copy to current
        cp -f "$configure_options_file" "config/configure_options/current"
    fi

    # Check that the options are in the correct order
    configure_options_are_ok="$(CheckOptions config/configure_options/current)"
    if test "$configure_options_are_ok" != ""; then

        echo " " 1>&2
        echo "===============================================================" 1>&2
        echo "Error message from autogen.sh:" 1>&2
        echo " "  1>&2
        echo $configure_options_are_ok 1>&2
        echo " "  1>&2
        echo "===============================================================" 1>&2
        
        # Fail, go back to start of while loop
        continue
    fi

    # Ask if these options are OK
    echo " "
    echo "Configure options are: "
    cat "config/configure_options/current"
    echo 
    if YesNoRead "Is this OK?" "y"; then
        accept_configure_options="true"
    else
        accept_configure_options="false"
    fi

done

echo
echo
echo


#====================================================================
# Start actual build process
#====================================================================

# Call real autogen
./non_interactive_autogen.sh "$@" -b $build_dir -C $oomph_root -c "${oomph_root}config/configure_options/current"

echo " "
echo "autogen.sh has finished! If you can't spot any error messages" 
echo "above this, oomph-lib should now be ready to use... " 
echo " " 
echo "If you encounter any problems, please study the installation" 
echo "instructions and the FAQ before contacting the developers. " 
echo " "
echo "To run self tests use \"make check -k\" or ./bin/parallel_self_test.py"