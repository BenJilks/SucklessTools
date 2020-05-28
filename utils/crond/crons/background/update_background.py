#!/usr/bin/python

import urllib.request
import json, sys, time
from os import path
import os

bing_iotd = 'https://www.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1&mkt=en-US'

try:
    data_raw = urllib.request.urlopen(bing_iotd).read()
    data = json.loads(data_raw)
    did_work = True
except:
    print('Error: Could not fetch image data')
    exit(-1)

images = data['images']
if len(images) < 0:
    sys.stderr.write('Error: no images found')
    exit(-1)

url = 'https://bing.com' + images[0]['url']
title = images[0]['title']

title = title\
        .replace(',', '')\
        .replace('.', '')\
        .replace(' ', '_')

home = path.expanduser('~')
img_out_path = home + '/backgrounds/' + title + '.jpg'

if not path.exists(img_out_path):
    img_data = urllib.request.urlopen(url).read()
    img_out = open(img_out_path, 'wb')
    img_out.write(img_data)
    img_out.close()

try:
	os.remove(home + '/backgrounds/current')
except:
	pass

os.link(img_out_path, home + '/backgrounds/current')
os.system('feh --bg-fill ' + home + '/backgrounds/current')
print('Updated background to ' + img_out_path)
exit(0)

