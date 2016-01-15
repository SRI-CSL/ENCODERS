#!/bin/bash
cd ..
rm cov-int.tgz
tar czvf cov-int.tgz cov-int
curl --form token=lodhL54-ElejZWOKrWDjcQ \
  --form email=minyounk@gmail.com \
  --form file=@cov-int.tgz\
  --form version="V1" \
  --form description="ENCODERS" \
  https://scan.coverity.com/builds?project=SRI-CSL%2FENCODERS
