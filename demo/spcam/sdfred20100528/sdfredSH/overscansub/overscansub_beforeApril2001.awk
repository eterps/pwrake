#awk script of making the input file list
#for overscansub.csh

# Created by Masami Ouchi
#       August 12, 1999
# Revised by Masami Ouchi
#       March 14, 2000
# Revised by Masayuki Miyazaki

#si001s
{si001s_c0 = 1 }
{si001s_c1 = 70 }

#si002s
{si002s_c0 = 1 }
{si002s_c1 = 70 }

#si005s
{si005s_c0 = 1 }
{si005s_c1 = 70 }

#si006s
{si006s_c0 = 1977 }
{si006s_c1 = 2046 }

#w67c1
{w67c1_c0 = 1 }
{w67c1_c1 = 70 }

#w6c1
{w6c1_c0 = 1 }
{w6c1_c1 = 70 }

#w93c2
{w93c2_c0 = 1977 }
{w93c2_c1 = 2046 }

#w9c2
{w9c2_c0 = 1977 }
{w9c2_c1 = 2046 }

#w4c5
{w4c5_c0 = 1977 }
{w4c5_c1 = 2046 }

#w7c3
{w7c3_c0 = 1}
{w7c3_c1 = 70 }


$1 ~/(si001s)+/ {print $1" "si001s_c0" "si001s_c1" o_"$1}
$1 ~/(si002s)+/ {print $1" "si002s_c0" "si002s_c1" o_"$1}
$1 ~/(si005s)+/ {print $1" "si005s_c0" "si005s_c1" o_"$1}
$1 ~/(si006s)+/ {print $1" "si006s_c0" "si006s_c1" o_"$1}
$1 ~/(w67c1)+/ {print $1" "w67c1_c0" "w67c1_c1" o_"$1}
$1 ~/(w6c1)+/ {print $1" "w6c1_c0" "w6c1_c1" o_"$1}
$1 ~/(w93c2)+/ {print $1" "w93c2_c0" "w93c2_c1" o_"$1}
$1 ~/(w9c2)+/ {print $1" "w9c2_c0" "w9c2_c1" o_"$1}
$1 ~/(w4c5)+/ {print $1" "w4c5_c0" "w4c5_c1" o_"$1}
$1 ~/(w7c3)+/ {print $1" "w7c3_c0" "w7c3_c1" o_"$1}
