#!/bin/bash
for ((i=1;i<=40;i+=1)); do
  ./client.py &
done
