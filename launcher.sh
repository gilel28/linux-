#!/bin/bash

NUM_DECRYPTERS=${1:-3}

# 1. מחיקת כל הפייפים הישנים וקבצים זמניים
sudo find /mnt/mta/ -type p -delete 2>/dev/null
sudo rm -f /mnt/mta/decoder_unit_pipe_* /mnt/mta/server_pipe

# 2. יצירת קובץ קונפיגורציה (אם לא קיים)
echo "PASSWORD_LENGTH=24" | sudo tee /mnt/mta/mtacrypt.conf > /dev/null
sudo chmod 666 /mnt/mta/mtacrypt.conf

# 3. תיקון הרשאות לכל הקבצים והפייפים בתיקייה
sudo chmod 666 /mnt/mta/* 2>/dev/null
sudo find /mnt/mta -type p -exec chmod 666 {} \; 2>/dev/null
sudo chmod 777 /mnt/mta

# 4. עצירת קונטיינרים קיימים (לא מוחק images)
sudo docker rm -f cipher_core &>/dev/null
for i in $(seq 1 $NUM_DECRYPTERS); do
    sudo docker rm -f decoder_unit$i &>/dev/null
done

# 5. הפעלת האנקריפטר (הגרסה שלך)
ENCRYPTER_ID=$(sudo docker run --pull=never -d \
    --name cipher_core \
    -v /mnt/mta:/mnt/mta \
    -v /var/log/cipher_core:/var/log \
    custom/image_encryptor:latest)
echo "Encryptor ${ENCRYPTER_ID} starting..."
sleep 3

# 6. הפעלת הדקריפטרים שלך
for i in $(seq 1 $NUM_DECRYPTERS); do
    DECRYPTER_ID=$(sudo docker run --pull=never -d \
        --name decoder_unit$i \
        -v /mnt/mta:/mnt/mta \
        -v /var/log/decoder_unit$i:/var/log \
        custom/image_decryptor:latest)
    echo "Decrypter #${i} (${DECRYPTER_ID}) starting..."
done

echo "System initialized with $NUM_DECRYPTERS decoder_units"

# 7. ניקוי dangling images (אופציונלי)
sudo docker image prune -f > /dev/null 2>&1
