#!/bin/sh

ORANGE="../orange --host localhost --port 61413 --username admin --password admin "; 

${ORANGE} list '*' '{}'
${ORANGE} call '/test' echo '{"foo":"bar"}'
${ORANGE} call '/test' exit '{}'
