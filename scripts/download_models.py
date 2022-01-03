#!/usr/bin/env python
import csv
import os

os.system("mkdir -p build/models")

with open('scripts/coqui_releases.csv') as csvfile:
    reader = csv.reader(csvfile)
    for line_index, row in enumerate(reader):
        if line_index == 0:
            continue
        code = row[0]
        release = row[1]
        print('Downloading release %s' % release)
        download_status = os.system(
            "gh release download %s --repo=coqui-ai/STT-models --dir=build/models/%s" % (release, code))
        if download_status != 0:
            print("Download failed")
            exit(download_status)
