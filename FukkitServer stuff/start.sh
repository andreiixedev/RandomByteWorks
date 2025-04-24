#!/bin/bash

VERSION="1.21.5"
PAPER_API="https://api.papermc.io/v2/projects/paper/versions/$VERSION"

# Obține JSON cu toate build-urile disponibile
LATEST_BUILD_JSON=$(curl -s "$PAPER_API/builds")

# Extrage ultimul build (după număr)
LATEST_ENTRY=$(echo "$LATEST_BUILD_JSON" | jq '.builds | sort_by(.build) | last')

# Extrage numele fișierului și numărul buildului
JAR_NAME=$(echo "$LATEST_ENTRY" | jq -r '.downloads.application.name')
BUILD_NUMBER=$(echo "$LATEST_ENTRY" | jq -r '.build')

# Link complet de descărcare
DOWNLOAD_URL="https://api.papermc.io/v2/projects/paper/versions/$VERSION/builds/$BUILD_NUMBER/downloads/$JAR_NAME"

# Verifică dacă fișierul există deja
if [ -f "$JAR_NAME" ]; then
    echo "Ultimul build ($JAR_NAME) este deja descărcat."
else
    echo "Se descarcă ultimul build PaperMC $VERSION (Build $BUILD_NUMBER): $JAR_NAME"
    curl -o "$JAR_NAME" "$DOWNLOAD_URL"

    # Șterge versiunile mai vechi (care încep cu paper-1.21.5- și nu sunt cea curentă)
    for f in paper-$VERSION-*.jar; do
        if [[ "$f" != "$JAR_NAME" ]]; then
            echo "Șterg vechiul fișier $f..."
            rm "$f"
        fi
    done
fi

# Rulează serverul
echo "Pornesc serverul cu $JAR_NAME..."
java -Xmx3300M -XX:+UseG1GC -XX:+ParallelRefProcEnabled -XX:MaxGCPauseMillis=200 \
-XX:+UnlockExperimentalVMOptions -XX:+DisableExplicitGC -XX:+AlwaysPreTouch \
-XX:G1NewSizePercent=40 -XX:G1MaxNewSizePercent=50 -XX:G1HeapRegionSize=16M \
-XX:G1ReservePercent=15 -XX:G1HeapWastePercent=5 -XX:G1MixedGCCountTarget=4 \
-XX:InitiatingHeapOccupancyPercent=20 -XX:G1MixedGCLiveThresholdPercent=90 \
-XX:G1RSetUpdatingPauseTimePercent=5 -XX:SurvivorRatio=32 -XX:+PerfDisableSharedMem \
-XX:MaxTenuringThreshold=1 -jar "$JAR_NAME" --nogui