#!/bin/bash
for dpi in 150 300 600 1200
do
  convert -background none -density $dpi fmigo-logo-plain.svg fmigo-logo-${dpi}dpi.png
  zopflipng fmigo-logo-${dpi}dpi.png -y fmigo-logo-${dpi}dpi.png
done
