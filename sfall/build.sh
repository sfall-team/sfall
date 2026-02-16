#!/opt/homebrew/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

TOOLCHAIN_MODE="${SFALL_TOOLCHAIN_MODE:-vs14_hybrid}"
if [[ "$TOOLCHAIN_MODE" != "vs14_hybrid" ]]; then
  echo "Unsupported SFALL_TOOLCHAIN_MODE='$TOOLCHAIN_MODE'. Only 'vs14_hybrid' is supported."
  exit 1
fi

WINE="${WINE:-wine}"
WINEDEBUG="${WINEDEBUG:--all}"
SFALL_WINEPREFIX="${SFALL_WINEPREFIX:-/Users/mark/Applications/Sikarugir/devFO2.app/Contents/SharedSupport/prefix}"

SFALL_VS14_VC_ROOT="${SFALL_VS14_VC_ROOT:-C:\\PROG~5P2\\MICR~HMH.0\\VC}"
SFALL_VS14_CL="${SFALL_VS14_CL:-C:\\PROG~5P2\\MICR~HMH.0\\VC\\bin\\cl.exe}"
SFALL_VS14_LINK="${SFALL_VS14_LINK:-C:\\PROG~5P2\\MICR~HMH.0\\VC\\bin\\link.exe}"
SFALL_VS14_RC="${SFALL_VS14_RC:-C:\\ssl\\tools\\my_msvc\\kits\\10\\bin\\10.0.26100.0\\x64\\rc.exe}"
SFALL_UCRT_INCLUDE_ROOT="${SFALL_UCRT_INCLUDE_ROOT:-C:\\ssl\\tools\\my_msvc\\kits\\10\\Include\\10.0.26100.0}"
SFALL_UCRT_LIB_ROOT="${SFALL_UCRT_LIB_ROOT:-C:\\ssl\\tools\\my_msvc\\kits\\10\\Lib\\10.0.26100.0}"

# DXSDK selection per upstream README:
# - Base include/libs from June 2010
# - ddraw.lib from February 2010
# - dinput.lib from August 2007
DXSDK_COLLECTION_ROOT="${DXSDK_COLLECTION_ROOT:-$ROOT/../../DXSDK_Collection}"
DXSDK_JUN2010="${DXSDK_JUN2010:-$DXSDK_COLLECTION_ROOT/DXSDK_Jun2010}"
DXSDK_FEB2010="${DXSDK_FEB2010:-$DXSDK_COLLECTION_ROOT/DXSDK_Feb2010}"
DXSDK_AUG2007="${DXSDK_AUG2007:-$DXSDK_COLLECTION_ROOT/DXSDK_Aug2007}"

DXINC_DIR="${DXINC_DIR:-$DXSDK_JUN2010/Include}"
DXLIB_JUN2010_DIR="${DXLIB_JUN2010_DIR:-$DXSDK_JUN2010/Lib/x86}"
DXLIB_FEB2010_DIR="${DXLIB_FEB2010_DIR:-$DXSDK_FEB2010/Lib/x86}"
DXLIB_AUG2007_DIR="${DXLIB_AUG2007_DIR:-$DXSDK_AUG2007/Lib/x86}"

for p in "$DXINC_DIR" "$DXLIB_JUN2010_DIR" "$DXLIB_FEB2010_DIR" "$DXLIB_AUG2007_DIR"; do
  if [[ ! -d "$p" ]]; then
    echo "Missing required DXSDK directory: $p"
    exit 1
  fi
done

if [[ ! -f "$DXLIB_FEB2010_DIR/ddraw.lib" ]]; then
  echo "Missing ddraw.lib (expected Feb2010): $DXLIB_FEB2010_DIR/ddraw.lib"
  exit 1
fi
if [[ ! -f "$DXLIB_AUG2007_DIR/dinput.lib" ]]; then
  echo "Missing dinput.lib (expected Aug2007): $DXLIB_AUG2007_DIR/dinput.lib"
  exit 1
fi

check_win_file() {
  local path="$1"
  if ! WINEPREFIX="$SFALL_WINEPREFIX" WINEDEBUG="$WINEDEBUG" "$WINE" cmd /c "if exist $path (exit /b 0) else (exit /b 1)" >/dev/null 2>&1; then
    echo "Missing required Windows file: $path"
    exit 1
  fi
}

winepath_w() {
  WINEPREFIX="$SFALL_WINEPREFIX" WINEDEBUG="$WINEDEBUG" winepath -w "$1"
}

run_wine_tool() {
  WINEPREFIX="$SFALL_WINEPREFIX" \
  WINEDEBUG="$WINEDEBUG" \
  INCLUDE="$INCLUDE_WIN" \
  LIB="$LIB_WIN" \
  LIBPATH="$LIB_WIN" \
  WINEPATH="$WINEPATH_WIN" \
  "$WINE" "$@"
}

check_win_file "$SFALL_VS14_CL"
check_win_file "$SFALL_VS14_LINK"

OUTDIR="$ROOT/build"
OBJDIR="$OUTDIR/obj"
mkdir -p "$OBJDIR"

ROOT_W="$(winepath_w "$ROOT")"
OBJDIR_W="$(winepath_w "$OBJDIR")"
DXINC_W="$(winepath_w "$DXINC_DIR")"
DXLIB_JUN2010_W="$(winepath_w "$DXLIB_JUN2010_DIR")"
DXLIB_FEB2010_W="$(winepath_w "$DXLIB_FEB2010_DIR")"
DXLIB_AUG2007_W="$(winepath_w "$DXLIB_AUG2007_DIR")"
DDRAW_FEB2010_W="$(winepath_w "$DXLIB_FEB2010_DIR/ddraw.lib")"
DINPUT_AUG2007_W="$(winepath_w "$DXLIB_AUG2007_DIR/dinput.lib")"

INCLUDE_WIN="${SFALL_VS14_VC_ROOT}\\include;${SFALL_VS14_VC_ROOT}\\atlmfc\\include;${SFALL_UCRT_INCLUDE_ROOT}\\shared;${SFALL_UCRT_INCLUDE_ROOT}\\ucrt;${SFALL_UCRT_INCLUDE_ROOT}\\um;${DXINC_W};${ROOT_W}"
LIB_WIN="${SFALL_VS14_VC_ROOT}\\lib;${SFALL_VS14_VC_ROOT}\\atlmfc\\lib;${SFALL_UCRT_LIB_ROOT}\\ucrt\\x86;${SFALL_UCRT_LIB_ROOT}\\um\\x86;${DXLIB_JUN2010_W};${DXLIB_FEB2010_W};${DXLIB_AUG2007_W}"
WINEPATH_WIN="$(dirname "$SFALL_VS14_CL");$(dirname "$SFALL_VS14_LINK");$(dirname "$SFALL_VS14_RC")"

mapfile -t SOURCES < <(grep -oE 'ClCompile Include="[^"]+"' ddraw.vcxproj | sed -E 's/^ClCompile Include="//; s/"$//')

if [[ "${#SOURCES[@]}" -eq 0 ]]; then
  echo "No sources found in ddraw.vcxproj."
  exit 1
fi

echo "Building sfall with toolchain mode '$TOOLCHAIN_MODE'..."
echo "Found ${#SOURCES[@]} source files."

# ReleaseXP-oriented flags from ddraw.vcxproj, mapped for standalone cl/link calls.
CFLAGS=(
  /nologo /c
  /O2 /Ob2 /Oi /Ot /Oy /GT
  /GL
  /GF
  /MT
  /EHsc
  /GS-
  /GR-
  /Zc:wchar_t-
  /Zc:threadSafeInit-
  /Gz
  /Gy
  /fp:fast
  /arch:SSE
  /W3
  /FI"stdafx.h"
  /I"$DXINC_W"
  /I"$ROOT_W"
  /DINITGUID
  /DWIN32
  /DNDEBUG
  /D_CONSOLE
  /D_CRT_SECURE_NO_WARNINGS
  /D_USING_V110_SDK71_
)

OBJS=()
for src in "${SOURCES[@]}"; do
  src="${src//$'\r'/}"
  src="${src//\\//}"
  src="${src#./}"

  obj_rel="${src//\//__}"
  obj="${obj_rel%.cpp}.obj"
  obj_w="$OBJDIR_W\\$obj"

  src_u="$ROOT/$src"
  if [[ ! -f "$src_u" ]]; then
    echo "Missing source: $src_u"
    exit 1
  fi
  src_w="$(winepath_w "$src_u")"

  echo "cl $src"
  run_wine_tool "$SFALL_VS14_CL" "${CFLAGS[@]}" /Fo"$obj_w" "$src_w"
  OBJS+=("$obj_w")
done

RES_W=""
if [[ -f "$ROOT/version.rc" ]]; then
  if WINEPREFIX="$SFALL_WINEPREFIX" WINEDEBUG="$WINEDEBUG" "$WINE" cmd /c "if exist $SFALL_VS14_RC (exit /b 0) else (exit /b 1)" >/dev/null 2>&1; then
    echo "rc version.rc"
    RES_U="$OBJDIR/version.res"
    RES_W="$(winepath_w "$RES_U")"
    run_wine_tool "$SFALL_VS14_RC" /nologo /fo"$RES_W" "$(winepath_w "$ROOT/version.rc")"
  else
    echo "Warning: rc.exe not found at '$SFALL_VS14_RC'; skipping version.rc"
  fi
fi

DEF_U="$ROOT/exports.def"
if [[ ! -f "$DEF_U" ]]; then
  echo "Missing exports.def at $DEF_U"
  exit 1
fi
DEF_W="$(winepath_w "$DEF_U")"

OUT_U="$OUTDIR/ddraw.dll"
OUT_W="$(winepath_w "$OUT_U")"

LFLAGS=(
  /nologo
  /DLL
  /OUT:"$OUT_W"
  /DEF:"$DEF_W"
  /LIBPATH:"$DXLIB_JUN2010_W"
  /LIBPATH:"$DXLIB_FEB2010_W"
  /LIBPATH:"$DXLIB_AUG2007_W"
  /SUBSYSTEM:CONSOLE,5.01
  /LTCG
  /OPT:REF
  /OPT:ICF
  /SAFESEH:NO
  /BASE:0x11000000
  /DYNAMICBASE:NO
  /DELAYLOAD:ws2_32.dll
  /DELAYLOAD:d3d9.dll
  /DELAYLOAD:d3dx9_43.dll
  /EMITPOGOPHASEINFO
  /MACHINE:X86
)

LIBS=(
  d3d9.lib
  d3dx9.lib
  "$DINPUT_AUG2007_W"
  "$DDRAW_FEB2010_W"
  Strmiids.lib
  ws2_32.lib
  delayimp.lib
  kernel32.lib
  user32.lib
  gdi32.lib
  winspool.lib
  comdlg32.lib
  advapi32.lib
  shell32.lib
  ole32.lib
  oleaut32.lib
  uuid.lib
  odbc32.lib
  odbccp32.lib
)

echo "link ddraw.dll"
if [[ -n "$RES_W" ]]; then
  run_wine_tool "$SFALL_VS14_LINK" "${LFLAGS[@]}" "${OBJS[@]}" "$RES_W" "${LIBS[@]}"
else
  run_wine_tool "$SFALL_VS14_LINK" "${LFLAGS[@]}" "${OBJS[@]}" "${LIBS[@]}"
fi

echo "Built: $OUT_U"
