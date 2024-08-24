wget -qO- --no-check-certificate \
    'https://air.cnemc.cn:18007/CityData/GetAQIDataPublishLive?cityName=%E5%8C%97%E4%BA%AC%E5%B8%82' |
sed -e 's/{//g' \
    -e 's/}./\n/g' |
sed -E \
    -e 's/.*"TimePoint":"([^"]*)".*"PositionName":"([^"]*)".*"PM2_5":"([^"]*)".*/\1,\2,\3/' \
    -e "s/([0-9]{4}(-[0-9]{1,2}){2})T(([0-9]{2}:){2}[0-9]{2})/\1 \3/"