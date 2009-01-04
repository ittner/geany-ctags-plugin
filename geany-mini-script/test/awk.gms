{
 tmp = $1
 $1  = $2 
 $2  = $3
 $3  = tmp
 print NR,$0
}