#!/bin/sh

ORANGE="../orange --host localhost --port 61413 --username admin --password admin "; 

${ORANGE} list '*' '{}'
${ORANGE} call '/test' echo '[]'
${ORANGE} call '/test' echo '{"foo":"bar"}'

sleep 1
${ORANGE} call '/test' exit '{}'
