/**
 *       @file  utlGTest.h
 *      @brief  
 *
 *
 *     @author  Dr. Jian Cheng (JC), jian.cheng.1983@gmail.com
 *
 *   @internal
 *     Created  "08-12-2014
 *    Revision  1.0
 *    Compiler  gcc/g++
 *     Company  IDEA@UNC-CH
 *   Copyright  Copyright (c) 2014, Jian Cheng
 *
 * =====================================================================================
 */
#ifndef __utlGTest_h
#define __utlGTest_h

#include "gtest/gtest.h"

/** @addtogroup utlHelperFunctions
@{ */

#define EXPECT_NEAR_COMPLEX(val1, val2, eps) \
  do { EXPECT_NEAR(std::abs(val1-val2), 0.0, eps) << "val1="<<val1 <<", val2=" <<val2; } while(0)

#define EXPECT_NEAR_RELATIVE(val1, val2, eps) \
  do { EXPECT_NEAR(val1,val2, eps*std::fabs(val2)); } while (0)

#define EXPECT_NEAR_VECTOR_COMPLEX(vec1, vec2, N, eps) \
  do { for ( int i = 0; i < N; i += 1 ) \
          EXPECT_NEAR(std::abs((vec1)[i]-(vec2)[i]), 0, eps) << "index = " << i << ", vec1[i]="<<vec1[i] <<", vec2[i]=" <<vec2[i]; } while (0)

#define EXPECT_NEAR_VECTOR(vec1, vec2, N, eps) \
  do { for ( int i = 0; i < N; i += 1 ) \
          EXPECT_NEAR((vec1)[i], (vec2)[i], eps) << "index = " << i; } while (0)

#define EXPECT_NEAR_MATRIX_COMPLEX(mat1, mat2, Row, Col, eps) \
  do { for ( int i = 0; i < Row; i += 1 ) \
          for ( int j = 0; j < Col; j += 1 ) \
              EXPECT_NEAR(std::abs((mat1)(i,j)-(mat2)(i,j)), 0, eps) << "index = ("<< i<<","<<j<<"), mat1(i,j)=" << mat1(i,j) << ", mat2(i,j)=" << mat2(i,j); } while (0)

#define EXPECT_NEAR_MATRIX(mat1, mat2, Row, Col, eps) \
  do { for ( int i = 0; i < Row; i += 1 ) \
          for ( int j = 0; j < Col; j += 1 ) \
            EXPECT_NEAR((mat1)(i,j), (mat2)(i,j), eps) << "index = ("<< i<<","<<j<<")"; } while (0)

#define EXPECT_NEAR_STDVECTOR(vec1,vec2,eps) \
  do { EXPECT_EQ((vec1).size(), (vec2).size()); \
       EXPECT_NEAR_VECTOR(vec1, vec2, (vec1).size(), eps); } while (0)

#define EXPECT_NEAR_UTLVECTOR(vec1,vec2,eps) \
  do { EXPECT_EQ((vec1).Size(), (vec2).Size()); \
       EXPECT_NEAR_VECTOR(vec1, vec2, (vec1).Size(), eps); } while (0)

#define EXPECT_NEAR_VNLVECTOR(vec1,vec2,eps) EXPECT_NEAR_STDVECTOR(vec1,vec2,eps)

#define EXPECT_NEAR_VNLMATRIX(mat1,mat2,eps) \
  do { EXPECT_EQ((mat1).rows(), (mat2).rows()); \
       EXPECT_EQ((mat1).cols(), (mat2).cols()); \
       EXPECT_NEAR_MATRIX(mat1, mat2, (mat1).rows(), (mat1).cols(), eps); } while (0)

#define EXPECT_NEAR_UTLMATRIX(mat1,mat2,eps) \
  do { EXPECT_EQ((mat1).Rows(), (mat2).Rows()); \
       EXPECT_EQ((mat1).Cols(), (mat2).Cols()); \
       EXPECT_NEAR_MATRIX(mat1, mat2, (mat1).Rows(), (mat1).Cols(), eps); } while (0)

    /** @} */
#endif 
