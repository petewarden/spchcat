#!/usr/bin/env python
import csv
import os

LANGUAGES_DIR = "build/models"

os.system("mkdir -p %s" % LANGUAGES_DIR)

with open('scripts/coqui_releases.csv') as csvfile:
    reader = csv.reader(csvfile)
    for line_index, row in enumerate(reader):
        if line_index == 0:
            continue
        code = row[0]
        release = row[1]
        output_dir = "%s/%s" % (LANGUAGES_DIR, code)
        print('Downloading release %s to %s' % (release, output_dir))
        download_status = os.system(
            "gh release download %s --repo=coqui-ai/STT-models --dir=%s" % (release, output_dir))
        if download_status != 0:
            print("Download failed")
            exit(download_status)
        # Delete large, unused files as soon as they're downloaded, to save space
        # on size-limited systems like the Raspberry Pi.
        # This logic should be kept in sync with the same in create_deb_package.sh.
        os.system("find %s -type f -name '*.pb*' -delete" % output_dir)
        os.system("find %s -iname '*.scorer' -size +150M -delete" % output_dir)