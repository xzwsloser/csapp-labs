/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 *  loser  Uxxx
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  // 异或的定义方式
  int res = ~((~(x & (~y))) & (~((~x) & y)));
  return res;
}
/* 
 * tmin - return minimum two's complement integer(返回最小的补码值)
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  // 最小的补码表示的数字 1 + 31个 0
  int res = 1 << 31;
  return res;

}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) { // 自己模拟
  // int xPlusOne = x + 1;  // 此时 xplusOne 相当于 ~x
	// int notTmaxOrMinusOne = !(~(x ^ xPlusOne));  // 此时 notTmaxOrMinusOne = 1,否则等于 0
	// return notTmaxOrMinusOne ^ !xPlusOne;  // 此时 !xPlusOne = 1 
  // 或者可以使用 x = ~(x + 1)  01111  --> 10000 , 但是需要排除 11111 的可能性
  // 第一个条件  x ^ (~(x + 1)) == 0 , 也就是  !(x ^ (~(x + 1))) = 1
  // 第二个条件  x + 1 != 0     ,也就是 !!((x + 1) ^ 0x0) = 1
  return !(x ^ (~(x + 1))) & !!((x + 1) ^ 0x00);
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  // 判断所有奇数位置为 1
  int mask = 0xAA;
  int fourA = mask + (mask << 8);
  int eightA = fourA + (fourA << 16);
  // 注意此时利用 0xAA -> 0xAAAA -> 0xAAAAAAAA,本来就有 32 位数没有溢出
  return !((x & eightA) ^ eightA); 
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  // return -x
  // 补码 = 反码 + 1
  return ~x + 1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  // 0x30 --> 0x00110000
  // 0x39 --> 0x00111001
  // 0x00110000 -- 0x00111001  要求就是 不可以超过 1100 并且不可以超过,也就是 只可以最后一位为 0 1010 
  int mask1 = ~0xFF;
  // 可以使用 & 运算进行截取操作,利用 ^ 和 !! 进行对比操作即可 , 利用 >> 进行一个位的移动
  return !((mask1&x)+((0xF0&x)^0x30)+!((12&x)^12)+!((10&x)^10));
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  // x 为真就可以返回 y , 为假的就会返回 z
  int mask1 = !x + (~0);  // x = 0 -->  !x = 1 + 111 --> 0000000
  int mask2 = ~mask1;  // x = 0 -> 全 1 , x = 1 -> 全 0
  // 一个数字 & 0x0000 就会得到 0x00 , 同时 &上 0xFFFF 就会得到自己
  return (y & mask1) | (z & mask2); 
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
    // x: 4   0000 0000 0000 0100
    // y: 5   0000 0000 0000 0101
    // x^y:   0000 0000 0000 0001
    // negy:  1111 1111 1111 1011
    //x+negy: 1111 1111 1111 1111
    // syn:   1000 0000 0000 0000
    //flags1: 0000 0000 0000 0001
    //flags2: 0000 0000 0000 0000
    //flags3: 0000 0000 0000 0000
    //x&syn : 1000 0000 0000 0000
    //      : 0000 0000 0000 0000
    // !!   : 0000 0000 0000 0000
    // res  : (1 & (0000 0000 0000 0001) | 0)
    // res  : 0000 0000 0000 0001
    // int negy = (~y)+1;  // 此时 negy = -y
    // int syn = 1<<31;    // 也就是 syn = 0x8000
    // int flag1 = !!((x+negy)&syn); // x + negy 如果满足条件要么 = 0x0000 
    // int flag2 = !(x^y);  // flags = 1 说明 x == y , flags == 0 说明 x != y
    // int flag3 = (x^y)&syn; // flags = 0 说明没有问题 
    // return ( (!flag3)&(flag1|flag2) | !!((flag3)&(x&syn)) );
    // 1. x == y
    int cond = !(x ^ y);  // 满足条件 cond = 1
    // 2. x + , y -
    int signX = (x >> 31) & 1;  // 可以得到符号位 0
    int signY = (y >> 31) & 1;  // 可以得到符号位 1
    int cond2 = !((!signX) & signY); // 此时 cond = 0 就不会满足条件
    // 3. x - , y + 可行,此时如果直接进行运算可能溢出
    int cond3 = !!((signX) & (!signY));   // signX = 1 signY = 0 
    // 4. x,y同号,此时不会溢出
    int sign_x_y = ((x + (~y) + 1) >> 31) & 1;
    // x -y < 0 , 所以 sign_x_y == 1
    int cond4 = sign_x_y;  // 此时如果满足条件那么就有 cond4 = 1
    // 进行条件的综合
    // cond2 = 1 并且 (cond3 = 1 或者 cond4 = 1)
    int cond5 = (cond2) & ((cond3) | (cond4));
    return cond | cond5;
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  // 实现取反符号 !
  // 0x00 --> 1 , 其他 --> 0
  // x = ~x + 1   --> 0 
  // 注意此时如果 x = 0 , 那么左边就是 0
  // 对于符号位, 一个 0 一个 1 最终就会得到 1 , 所以可以通过 符号为的1算术右移来判断
  // 1111 1111  1111  1111 表示 -1
  // 对于非 0 的数字,一定含有符号位(如果符号位为0,那么相反数的符号为一定就为 1,可以拿到这一个符号位),所以对于符号位进行判断即可
  int res = ((x | (~x + 1)) >> 31) + 1;
  return res;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
              0 1 1 0 0
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            1 0 1 1
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  // 正数 x | 0     x
  // 负数 0 | ~x   ~x
  // int isZero = !x;   // 用于判断 x 是否 = 0
  int flag = x >> 31;  // 相当于获取符号位
  // 让正数和负数的最高为显示
  x = (flag & (~x)) | ((~flag) & x);
  // 首先判断高 16 为是否有数字
  int bit_16 , bit_8 , bit_4 , bit_2 , bit_1 , bit_0;
  // 注意此时如果高 16 为有数字,那么就会导致 !!x>>16 = 1 1 ^ 1 = 0 , !(1 ^ 1) = 1 ,bit_16 = 16
  // 但是如果没有数字,那么就会导致 bit_16 = 0; 不用继续判断了
  bit_16 = (!!(x >> 16)) << 4; 
  x >>= bit_16;
  bit_8 = (!!(x >> 8)) << 3; // 原理和上面一样
  x >>= bit_8;
  bit_4 = (!!(x >> 4)) << 2; // 原理和上面一样
  x >>= bit_4;
  bit_2 = (!!(x >> 2)) << 1; // 原理和上面一样
  x >>= bit_2;
  bit_1 = (!!(x >> 1)); // 原理和上面一样
  x >>= bit_1;
  bit_0 = x;  // 此时已经移动到末尾了,要么是 -1 要么是 0
  // int mask = ((!!x) << 31) >> 31;  // 掩码用于排除干扰
  int res = bit_16 + bit_8 + bit_4 + bit_2 + bit_1 + bit_0 + 1; // 最终的一个去掉的符号位
  return res;
  
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  // 首先取得各个位置
  unsigned s , expr , frac;
  s = (uf >> 31) & (0x1);  // 相当于只关心最后一位
  expr = (uf >> 23) &  (0xFF);  // 只用关心最后 8 位
  frac = (uf & 0x7FFFFF);
  // 开始判断
  // 1. 如果是 0
  if(expr == 0 && frac == 0)
  {
      return uf;  // uf == 0
  }
  // 2. 如果是无穷
  if(expr == 0xFF)
  {
    return uf;
  }
  // 3. 如果不是常规的
  if(expr == 0)
  {
    frac <<= 1;  // 相当于变成两倍
    return (s << 31) | frac;
  }
  // 正常情况 expr += 1 即可
  expr ++;
  return (s << 31) | (expr << 23) | frac;
  return 2;
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
  unsigned s , expr , frac;
  // 获取值
  s = (uf >> 31) & (0x1);
  expr = (uf >> 23) & (0xFF);
  frac = (uf & 0x7FFFFF);
  if(expr == 0 && s == 0) return 0;
  if(expr == 0xFF) return 1 << 31;
  if(expr == 0)  return 0;  // 此时 expr 很小
  int E = expr - 127; // 表示偏置量
  frac = frac | (1 << 23); // 补上前面的位置
  if(E > 31) return 1 << 31;  // 此时就会溢出
  if(E < 0)  return 0;

  if(E >= 23) frac <<= (E - 23); // 相当于位数比较大可以向左边移动
  else frac >>= (23 - E);  // 相当于截断
  
  if(s) return ~frac + 1;
  else return frac;
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
   // 听不懂放弃了
   if(x < -149) return 0;
   else if(x < -126) {
    int shift = 23 + (x + 126); // 相当于进行弥补操作
    return 1 << shift;
   } else if(x <= 127) {
    int expr = x + 127;
    return expr << 23;
   } else {
    return (0xFF) << 23;
   }
}
