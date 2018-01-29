#!/bin/sh

if [ $# != 3 ]; then
    echo
    echo "Usage: mkdmg.sh appname kind version"
    echo
    exit 1
fi

APPNAME="$1"
KIND="$2"
VERSION="$3"

if [ "$KIND" = "CLI" ] || [ "$KIND" = "cli" ]; then
    KIND="CLI"
elif [ "$KIND" = "GUI" ] || [ "$KIND" = "gui" ]; then
    KIND="GUI"
else
    echo
    echo "KIND must be either [CLI | cli] or [GUI | gui]"
    echo
    exit 1
fi

APPNAME_lower=`echo ${APPNAME} |awk '{print tolower($0)}'`
KIND_lower=`echo ${KIND} |awk '{print tolower($0)}'`
SIGNATURE="MediaArea.net"
FILES="tmp-${APPNAME}_${KIND}"
TEMPDMG="tmp-${APPNAME}_${KIND}.dmg"
FINALDMG="${APPNAME/ /}_${KIND}_${VERSION}_Mac.dmg"

# Clean up
rm -fr "${FILES}-Root"
rm -fr "${FILES}"
rm -f "${APPNAME}.pkg"
rm -f "${TEMPDMG}"
rm -f "${FINALDMG}"

echo
echo ========== Create the package ==========
echo

mkdir -p "${FILES}/.background"
cp ../../License.html "${FILES}"
#cp "../../Release/ReadMe_${KIND}_Mac.txt" "${FILES}/ReadMe.txt"
#cp "../../History_${KIND}.txt" "${FILES}/History.txt"
cp Logo_White.icns "${FILES}/.background"

if [ "$KIND" = "CLI" ]; then

    cd ../GNU/CLI
    if test -e ".libs/${APPNAME_lower}"; then
        mv -f ".libs/${APPNAME_lower}" .
    fi
    if ! test -x "${APPNAME_lower}"; then
        echo
        echo "${APPNAME_lower} can’t be found, or this file isn’t a executable."
        echo
        exit 1
    fi
    strip -u -r "${APPNAME_lower}"
    cd ../../Mac

    mkdir -p "${FILES}-Root/usr/local/bin"
    cp "../GNU/CLI/${APPNAME_lower}" "${FILES}-Root/usr/local/bin"
    codesign -f -s "Developer ID Application: ${SIGNATURE}" --verbose "${FILES}-Root/usr/local/bin/${APPNAME_lower}"

    pkgbuild --root "${FILES}-Root" --identifier "net.mediaarea.${APPNAME_lower}.mac-${KIND_lower}" --sign "Developer ID Installer: ${SIGNATURE}" --version "${VERSION}" "${FILES}/${APPNAME_lower}.pkg"
    codesign -f -s "Developer ID Application: ${SIGNATURE}" --verbose "${FILES}/${APPNAME_lower}.pkg"

fi

if [ "$KIND" = "GUI" ]; then

    cd ../Qt
    if ! test -e "${APPNAME}.app"; then
        echo
        echo "${APPNAME}.app can’t be found, or this file isn’t a executable."
        echo
        exit 1
    fi
    cd ../Mac

    cp -r "../Qt/${APPNAME}.app" "${FILES}"

    macdeployqt "${FILES}/${APPNAME}.app"

    # Qt 5.3 doesn’t handle the new version of Apple
    # signatures (Mac 10.9+), so we must determin which Qt
    # frameworks is used by MediaConch and sign them manually.
    for FRAMEWORK in `ls "${FILES}/${APPNAME}.app"/Contents/Frameworks |grep framework | sed "s/\.framework//"` ; do
        pushd "${FILES}/${APPNAME}.app/Contents/Frameworks/${FRAMEWORK}.framework"
        # Despite their misleading names, these directories
        # generated by macdeployqt must be deleted, or codesign
        # will fail.
        rm -fr _CodeSignature
        rm -fr Versions/Current/_CodeSignature
        # The trailing slash saga continues… codesign will
        # fail with "ln -s 5/ Current".
        ln -s 5 Versions/Current
        cp "${HOME}/Qt/5.3/clang_64/lib/${FRAMEWORK}.framework/Contents/Info.plist" Resources
        mv Resources Versions/Current
        ln -s Versions/Current/${FRAMEWORK}
        ln -s Versions/Current/Resources Resources
        if [ "$FRAMEWORK" = "QtPrintSupport" ] ; then
            sed -i '' 's/_debug//g' Resources/Info.plist
        fi
        popd
        codesign -f -s "Developer ID Application: ${SIGNATURE}" --verbose "${FILES}/${APPNAME}.app/Contents/Frameworks/${FRAMEWORK}.framework"
    done

    find "${FILES}/${APPNAME}.app/Contents/PlugIns" -name "*.dylib" -exec codesign -f -s "Developer ID Application: ${SIGNATURE}" --verbose "{}" \;

    codesign -f -s "Developer ID Application: ${SIGNATURE}" --verbose "${FILES}/${APPNAME}.app/Contents/MacOS/${APPNAME}"
    codesign -f -s "Developer ID Application: ${SIGNATURE}" --verbose "${FILES}/${APPNAME}.app"
fi

echo
echo ========== Create the disk image ==========
echo

# Check if an old image isn't already attached
DEVICE=$(hdiutil info |grep -B 1 "/Volumes/${APPNAME}" |egrep '^/dev/' | sed 1q | awk '{print $1}')
test -e "$DEVICE" && hdiutil detach -force "${DEVICE}"

hdiutil create "${TEMPDMG}" -ov -format UDRW -volname "${APPNAME}" -srcfolder "${FILES}"
DEVICE=$(hdiutil attach -readwrite -noverify "${TEMPDMG}" | egrep '^/dev/' | sed 1q | awk '{print $1}')
sleep 2

cd "/Volumes/${APPNAME}"
if [ "$KIND" = "GUI" ]; then
    ln -s /Applications
fi
test -e .DS_Store && rm -fr .DS_Store
cd - >/dev/null

. Osascript_${KIND}.sh
osascript_Function

hdiutil detach "${DEVICE}"
sleep 2

echo
echo ========== Convert to compressed image ==========
echo
hdiutil convert "${TEMPDMG}" -format UDBZ -o "${FINALDMG}"

unset -v APPNAME APPNAME_lower KIND KIND_lower VERSION SIGNATURE
unset -v TEMPDMG FINALDMG FILES DEVICE
