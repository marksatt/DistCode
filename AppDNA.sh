#!/bin/bash

# AppDNA Script
# Copyright 2006 Chuck Houpt <chuck@habilis.net>. No rights reserved.

set -eux

# Only bundle the source for Release/Deployment builds.
if [ "$CONFIGURATION" != 'Release' -a "$CONFIGURATION" != 'Deployment' ] ; then exit ; fi

INSIDE_SOURCE="$TARGET_BUILD_DIR"/"$CONTENTS_FOLDER_PATH"/Source
PROJECT_NAME=`basename "$PROJECT_FILE_PATH"`

rm -rf "$INSIDE_SOURCE"
mkdir -p "$INSIDE_SOURCE"

# Copy the source tree, minus resources, into Content/Source.
rsync -a --cvs-exclude --extended-attributes \
--exclude 'build' --exclude '*.pbxuser' --exclude '*.mode1' --exclude '.git' \
--exclude '*.scriptSuite' --exclude '*.o' --exclude '*.d' \
--exclude '*.iconset' --exclude 'xcuserdata' --exclude '*.mode1v3' --exclude '*.perspectivev3' \
$@ \
"$SRCROOT"/ "$INSIDE_SOURCE"

# Check if the project has already been modified for embedding.
if ! grep 'path = ../Resources;' "$PROJECT_FILE_PATH"/project.pbxproj
then 

	# No, so modify the project to find resources in the Package's Resources directory.
	# Change the location of Info.plist, since it isn't like other resources. Add _Mod to
	# Current Project Version to identify derivitive apps.

	sed \
	-e '/name = Resources;/a\
	path = ../Resources;\
	' \
	-e 's/path = Info.plist; sourceTree = "<group>";/path = Info.plist; sourceTree = SOURCE_ROOT;/' \
	-e 's/CURRENT_PROJECT_VERSION = \(.*\);/CURRENT_PROJECT_VERSION = \1_Mutant;/g' \
	"$PROJECT_FILE_PATH"/project.pbxproj > "$INSIDE_SOURCE"/"$PROJECT_NAME"/project.pbxproj

# Check if the project is embedded.
elif [ -d "$SRCROOT"/../../../"$WRAPPER_NAME" ]
then
	# Project embedded, so syncrhonize the build result to the enclosing application.

	rsync -a --extended-attributes --exclude 'Source'  "$TARGET_BUILD_DIR"/"$WRAPPER_NAME"/ "$SRCROOT"/../../../"$WRAPPER_NAME"
fi

cd "$TARGET_BUILD_DIR"/"$CONTENTS_FOLDER_PATH"
zip -r Source.zip Source
rm -rf "$INSIDE_SOURCE"
