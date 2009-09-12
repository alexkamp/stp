/********************************************************************
 * AUTHORS: Vijay Ganesh
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
// -*- c++ -*-

#include "../AST/AST.h"
#include "../AST/ASTUtil.h"
#include <cassert>

namespace BEEV
{

  //error printing
  static void BVConstEvaluatorError(CONSTANTBV::ErrCode e, const ASTNode& t)
  {
    std::string ss("BVConstEvaluator:");
    ss += (const char*) BitVector_Error(e);
    FatalError(ss.c_str(), t);
  }

  ASTNode BeevMgr::BVConstEvaluator(const ASTNode& t)
  {
    ASTNode OutputNode;
    Kind k = t.GetKind();

    if (CheckSolverMap(t, OutputNode))
      return OutputNode;
    OutputNode = t;

    unsigned int inputwidth = t.GetValueWidth();
    unsigned int outputwidth = inputwidth;
    CBV output = NULL;

    CBV tmp0 = NULL;
    CBV tmp1 = NULL;

    //saving some typing. BVPLUS does not use these variables. if the
    //input BVPLUS has two nodes, then we want to avoid setting these
    //variables.
    if (1 == t.Degree())
      {
        tmp0 = BVConstEvaluator(t[0]).GetBVConst();
      }
    else if (2 == t.Degree() && k != BVPLUS)
      {
        tmp0 = BVConstEvaluator(t[0]).GetBVConst();
        tmp1 = BVConstEvaluator(t[1]).GetBVConst();
      }

    switch (k)
      {
      case UNDEFINED:
      case READ:
      case WRITE:
      case SYMBOL:
        FatalError("BVConstEvaluator: term is not a constant-term", t);
        break;
      case BVCONST:
        //FIXME Handle this special case better
        OutputNode = t;
        break;
      case BVNEG:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          CONSTANTBV::Set_Complement(output, tmp0);
          OutputNode = CreateBVConst(output, outputwidth);
          break;
        }
      case BVSX:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          //unsigned * out0 = BVConstEvaluator(t[0]).GetBVConst();
          unsigned t0_width = t[0].GetValueWidth();
          if (inputwidth == t0_width)
            {
              CONSTANTBV::BitVector_Copy(output, tmp0);
              OutputNode = CreateBVConst(output, outputwidth);
            }
          else
            {
              bool topbit_sign = (CONSTANTBV::BitVector_Sign(tmp0) < 0);

              if (topbit_sign)
                {
                  CONSTANTBV::BitVector_Fill(output);
                }
              CONSTANTBV::BitVector_Interval_Copy(output, tmp0, 0, 0, t0_width);
              OutputNode = CreateBVConst(output, outputwidth);
            }
          break;
        }

      case BVZX:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          unsigned t0_width = t[0].GetValueWidth();
          if (inputwidth == t0_width)
            {
              CONSTANTBV::BitVector_Copy(output, tmp0);
              OutputNode = CreateBVConst(output, outputwidth);
            }
          else
            {
              CONSTANTBV::BitVector_Interval_Copy(output, tmp0, 0, 0, t0_width);
              OutputNode = CreateBVConst(output, outputwidth);
            }
          break;
        }

      case BVLEFTSHIFT:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);

          // the shift is destructive, get a copy.
          CONSTANTBV::BitVector_Interval_Copy(output, tmp0, 0, 0, inputwidth);

          // get the number of bits to shift it.
          unsigned int shift = GetUnsignedConst(BVConstEvaluator(t[1]));

          CONSTANTBV::BitVector_Move_Left(output, shift);
          OutputNode = CreateBVConst(output, outputwidth);
          break;
        }
      case BVRIGHTSHIFT:
      case BVSRSHIFT:
        {
          bool msb = CONSTANTBV::BitVector_msb_(tmp0);

          output = CONSTANTBV::BitVector_Create(inputwidth, true);

          // the shift is destructive, get a copy.
          CONSTANTBV::BitVector_Interval_Copy(output, tmp0, 0, 0, inputwidth);

          // get the number of bits to shift it.
          unsigned int shift = GetUnsignedConst(BVConstEvaluator(t[1]));

          CONSTANTBV::BitVector_Move_Right(output, shift);

          if (BVSRSHIFT == k && msb)
            {
              // signed shift, and the number was originally negative.
              // Shift may be larger than the inputwidth.
              for (unsigned int i = 0; i < min(shift, inputwidth); i++)
                {
                  CONSTANTBV::BitVector_Bit_On(output, (inputwidth - 1 - i));
                }
              assert(CONSTANTBV::BitVector_Sign(output) == -1); //must be negative.
            }

          OutputNode = CreateBVConst(output, outputwidth);
          break;
        }

      case BVAND:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          CONSTANTBV::Set_Intersection(output, tmp0, tmp1);
          OutputNode = CreateBVConst(output, outputwidth);
          break;
        }
      case BVOR:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          CONSTANTBV::Set_Union(output, tmp0, tmp1);
          OutputNode = CreateBVConst(output, outputwidth);
          break;
        }
      case BVXOR:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          CONSTANTBV::Set_ExclusiveOr(output, tmp0, tmp1);
          OutputNode = CreateBVConst(output, outputwidth);
          break;
        }
      case BVSUB:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          bool carry = false;
          CONSTANTBV::BitVector_sub(output, tmp0, tmp1, &carry);
          OutputNode = CreateBVConst(output, outputwidth);
          break;
        }
      case BVUMINUS:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          CONSTANTBV::BitVector_Negate(output, tmp0);
          OutputNode = CreateBVConst(output, outputwidth);
          break;
        }
      case BVEXTRACT:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          tmp0 = BVConstEvaluator(t[0]).GetBVConst();
          unsigned int hi = GetUnsignedConst(BVConstEvaluator(t[1]));
          unsigned int low = GetUnsignedConst(BVConstEvaluator(t[2]));
          unsigned int len = hi - low + 1;

          CONSTANTBV::BitVector_Destroy(output);
          output = CONSTANTBV::BitVector_Create(len, false);
          CONSTANTBV::BitVector_Interval_Copy(output, tmp0, 0, low, len);
          outputwidth = len;
          OutputNode = CreateBVConst(output, outputwidth);
          break;
        }
        //FIXME Only 2 inputs?
      case BVCONCAT:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          unsigned t0_width = t[0].GetValueWidth();
          unsigned t1_width = t[1].GetValueWidth();
          CONSTANTBV::BitVector_Destroy(output);

          output = CONSTANTBV::BitVector_Concat(tmp0, tmp1);
          outputwidth = t0_width + t1_width;
          OutputNode = CreateBVConst(output, outputwidth);

          break;
        }
      case BVMULT:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          CBV tmp = CONSTANTBV::BitVector_Create(2* inputwidth , true);
          CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Multiply(tmp, tmp0, tmp1);

          if (0 != e)
            {
              BVConstEvaluatorError(e, t);
            }
          //FIXME WHAT IS MY OUTPUT???? THE SECOND HALF of tmp?
          //CONSTANTBV::BitVector_Interval_Copy(output, tmp, 0, inputwidth, inputwidth);
          CONSTANTBV::BitVector_Interval_Copy(output, tmp, 0, 0, inputwidth);
          OutputNode = CreateBVConst(output, outputwidth);
          CONSTANTBV::BitVector_Destroy(tmp);
          break;
        }
      case BVPLUS:
        {
          output = CONSTANTBV::BitVector_Create(inputwidth, true);
          bool carry = false;
          ASTVec c = t.GetChildren();
          for (ASTVec::iterator it = c.begin(), itend = c.end(); it != itend; it++)
            {
              CBV kk = BVConstEvaluator(*it).GetBVConst();
              CONSTANTBV::BitVector_add(output, output, kk, &carry);
              carry = false;
              //CONSTANTBV::BitVector_Destroy(kk);
            }
          OutputNode = CreateBVConst(output, outputwidth);
          break;
        }

        // SBVREM : Result of rounding the quotient towards zero. i.e. (-10)/3, has a remainder of -1
        // SBVMOD : Result of rounding the quotient towards -infinity. i.e. (-10)/3, has a modulus of 2.
        //          EXCEPT THAT if it divides exactly and the signs are different, then it's equal to the dividend.
      case SBVDIV:
      case SBVREM:
        {
          CBV quotient = CONSTANTBV::BitVector_Create(inputwidth, true);
          CBV remainder = CONSTANTBV::BitVector_Create(inputwidth, true);

          if (division_by_zero_returns_one && CONSTANTBV::BitVector_is_empty(tmp1))
            {
              // Expecting a division by zero. Just return one.
              OutputNode = CreateOneConst(outputwidth);
            }
          else
            {
              CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Divide(quotient, tmp0, tmp1, remainder);

              if (e != 0)
                {
                  cerr << "WARNING" << endl;
                  FatalError((const char*) CONSTANTBV::BitVector_Error(e));
                }

              if (SBVDIV == k)
                {
                  OutputNode = CreateBVConst(quotient, outputwidth);
                  CONSTANTBV::BitVector_Destroy(remainder);
                }
              else
                {
                  OutputNode = CreateBVConst(remainder, outputwidth);
                  CONSTANTBV::BitVector_Destroy(quotient);

                }
            }
          break;
        }

      case SBVMOD:
        {
          /* Definition taken from the SMTLIB website
             (bvsmod s t) abbreviates
             (let (?msb_s (extract[|m-1|:|m-1|] s))
             (let (?msb_t (extract[|m-1|:|m-1|] t))
             (ite (and (= ?msb_s bit0) (= ?msb_t bit0))
             (bvurem s t)
             (ite (and (= ?msb_s bit1) (= ?msb_t bit0))
             (bvadd (bvneg (bvurem (bvneg s) t)) t)
             (ite (and (= ?msb_s bit0) (= ?msb_t bit1))
             (bvadd (bvurem s (bvneg t)) t)
             (bvneg (bvurem (bvneg s) (bvneg t)))))))
          */

          assert(t[0].GetValueWidth() == t[1].GetValueWidth());

          bool isNegativeS = CONSTANTBV::BitVector_msb_(tmp0);
          bool isNegativeT = CONSTANTBV::BitVector_msb_(tmp1);

          CBV quotient = CONSTANTBV::BitVector_Create(inputwidth, true);
          CBV remainder = CONSTANTBV::BitVector_Create(inputwidth, true);
          tmp0 = CONSTANTBV::BitVector_Clone(tmp0);
          tmp1 = CONSTANTBV::BitVector_Clone(tmp1);

          if (division_by_zero_returns_one && CONSTANTBV::BitVector_is_empty(tmp1))
            {
              // Expecting a division by zero. Just return one.
              OutputNode = CreateOneConst(outputwidth);

            }
          else
            {
              if (!isNegativeS && !isNegativeT)
                {
                  // Signs are both positive
                  CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Div_Pos(quotient, tmp0, tmp1, remainder);
                  if (e != CONSTANTBV::ErrCode_Ok)
                    {
                      cerr << "Error code was:" << e << endl;
                      assert(e == CONSTANTBV::ErrCode_Ok);
                    }
                  OutputNode = CreateBVConst(remainder, outputwidth);
                }
              else if (isNegativeS && !isNegativeT)
                {
                  // S negative, T positive.
                  CBV tmp0b = CONSTANTBV::BitVector_Create(inputwidth, true);
                  CONSTANTBV::BitVector_Negate(tmp0b, tmp0);

                  CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Div_Pos(quotient, tmp0b, tmp1, remainder);

                  assert(e == CONSTANTBV::ErrCode_Ok);

                  CBV remb = CONSTANTBV::BitVector_Create(inputwidth, true);
                  CONSTANTBV::BitVector_Negate(remb, remainder);

                  bool carry = false;
                  CBV res = CONSTANTBV::BitVector_Create(inputwidth, true);
                  CONSTANTBV::BitVector_add(res, remb, tmp1, &carry);

                  OutputNode = CreateBVConst(res, outputwidth);

                  CONSTANTBV::BitVector_Destroy(tmp0b);
                  CONSTANTBV::BitVector_Destroy(remb);
                  CONSTANTBV::BitVector_Destroy(remainder);
                }
              else if (!isNegativeS && isNegativeT)
                {
                  // If s is positive and t is negative
                  CBV tmp1b = CONSTANTBV::BitVector_Create(inputwidth, true);
                  CONSTANTBV::BitVector_Negate(tmp1b, tmp1);

                  CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Div_Pos(quotient, tmp0, tmp1b, remainder);

                  assert(e == CONSTANTBV::ErrCode_Ok);

                  bool carry = false;
                  CBV res = CONSTANTBV::BitVector_Create(inputwidth, true);
                  CONSTANTBV::BitVector_add(res, remainder, tmp1, &carry);

                  OutputNode = CreateBVConst(res, outputwidth);

                  CONSTANTBV::BitVector_Destroy(tmp1b);
                  CONSTANTBV::BitVector_Destroy(remainder);
                }
              else if (isNegativeS && isNegativeT)
                {
                  // Signs are both negative
                  CBV tmp0b = CONSTANTBV::BitVector_Create(inputwidth, true);
                  CBV tmp1b = CONSTANTBV::BitVector_Create(inputwidth, true);
                  CONSTANTBV::BitVector_Negate(tmp0b, tmp0);
                  CONSTANTBV::BitVector_Negate(tmp1b, tmp1);

                  CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Div_Pos(quotient, tmp0b, tmp1b, remainder);
                  assert(e == CONSTANTBV::ErrCode_Ok);

                  CBV remb = CONSTANTBV::BitVector_Create(inputwidth, true);
                  CONSTANTBV::BitVector_Negate(remb, remainder);

                  OutputNode = CreateBVConst(remb, outputwidth);
                  CONSTANTBV::BitVector_Destroy(tmp0b);
                  CONSTANTBV::BitVector_Destroy(tmp1b);
                  CONSTANTBV::BitVector_Destroy(remainder);
                }
              else
                {
                  FatalError("never get called");
                }
            }

          CONSTANTBV::BitVector_Destroy(tmp0);
          CONSTANTBV::BitVector_Destroy(tmp1);
          CONSTANTBV::BitVector_Destroy(quotient);
        }
        break;

      case BVDIV:
      case BVMOD:
        {
          CBV quotient = CONSTANTBV::BitVector_Create(inputwidth, true);
          CBV remainder = CONSTANTBV::BitVector_Create(inputwidth, true);

          if (division_by_zero_returns_one && CONSTANTBV::BitVector_is_empty(tmp1))
            {
              // Expecting a division by zero. Just return one.
              OutputNode = CreateOneConst(outputwidth);
            }
          else
            {

              // tmp0 is dividend, tmp1 is the divisor
              //All parameters to BitVector_Div_Pos must be distinct unlike BitVector_Divide
              //FIXME the contents of the second parameter to Div_Pos is destroyed
              //As tmp0 is currently the same as the copy belonging to an ASTNode t[0]
              //this must be copied.
              tmp0 = CONSTANTBV::BitVector_Clone(tmp0);
              CONSTANTBV::ErrCode e = CONSTANTBV::BitVector_Div_Pos(quotient, tmp0, tmp1, remainder);
              CONSTANTBV::BitVector_Destroy(tmp0);

              if (0 != e)
                {
                  //error printing
                  if (counterexample_checking_during_refinement)
                    {
                      output = CONSTANTBV::BitVector_Create(inputwidth, true);
                      OutputNode = CreateBVConst(output, outputwidth);
                      bvdiv_exception_occured = true;

                      //  CONSTANTBV::BitVector_Destroy(output);
                      break;
                    }
                  else
                    {
                      BVConstEvaluatorError(e, t);
                    }
                } //end of error printing

              //FIXME Not very standard in the current scheme
              if (BVDIV == k)
                {
                  OutputNode = CreateBVConst(quotient, outputwidth);
                  CONSTANTBV::BitVector_Destroy(remainder);
                }
              else
                {
                  OutputNode = CreateBVConst(remainder, outputwidth);
                  CONSTANTBV::BitVector_Destroy(quotient);
                }
            }
          break;
        }
      case ITE:
        {
          ASTNode tmp0 = t[0]; // Should this run BVConstEvaluator??

          if (ASTTrue == tmp0)
            OutputNode = BVConstEvaluator(t[1]);
          else if (ASTFalse == tmp0)
            OutputNode = BVConstEvaluator(t[2]);
          else
            {
              cerr << tmp0;
              FatalError("BVConstEvaluator: ITE condiional must be either TRUE or FALSE:", t);
            }
        }
        break;
      case EQ:
        if (CONSTANTBV::BitVector_equal(tmp0, tmp1))
          OutputNode = ASTTrue;
        else
          OutputNode = ASTFalse;
        break;
      case BVLT:
        if (-1 == CONSTANTBV::BitVector_Lexicompare(tmp0, tmp1))
          OutputNode = ASTTrue;
        else
          OutputNode = ASTFalse;
        break;
      case BVLE:
        {
          int comp = CONSTANTBV::BitVector_Lexicompare(tmp0, tmp1);
          if (comp <= 0)
            OutputNode = ASTTrue;
          else
            OutputNode = ASTFalse;
          break;
        }
      case BVGT:
        if (1 == CONSTANTBV::BitVector_Lexicompare(tmp0, tmp1))
          OutputNode = ASTTrue;
        else
          OutputNode = ASTFalse;
        break;
      case BVGE:
        {
          int comp = CONSTANTBV::BitVector_Lexicompare(tmp0, tmp1);
          if (comp >= 0)
            OutputNode = ASTTrue;
          else
            OutputNode = ASTFalse;
          break;
        }
      case BVSLT:
        if (-1 == CONSTANTBV::BitVector_Compare(tmp0, tmp1))
          OutputNode = ASTTrue;
        else
          OutputNode = ASTFalse;
        break;
      case BVSLE:
        {
          signed int comp = CONSTANTBV::BitVector_Compare(tmp0, tmp1);
          if (comp <= 0)
            OutputNode = ASTTrue;
          else
            OutputNode = ASTFalse;
          break;
        }
      case BVSGT:
        if (1 == CONSTANTBV::BitVector_Compare(tmp0, tmp1))
          OutputNode = ASTTrue;
        else
          OutputNode = ASTFalse;
        break;
      case BVSGE:
        {
          int comp = CONSTANTBV::BitVector_Compare(tmp0, tmp1);
          if (comp >= 0)
            OutputNode = ASTTrue;
          else
            OutputNode = ASTFalse;
          break;
        }
      default:
        FatalError("BVConstEvaluator: The input kind is not supported yet:", t);
        break;
      }
    /*
      if(BVCONST != k){
      cerr<<inputwidth<<endl;
      cerr<<"------------------------"<<endl;
      t.LispPrint(cerr);
      cerr<<endl;
      OutputNode.LispPrint(cerr);
      cerr<<endl<<"------------------------"<<endl;
      }
    */
    UpdateSolverMap(t, OutputNode);
    //UpdateSimplifyMap(t,OutputNode,false);
    return OutputNode;
  } //End of BVConstEvaluator
}; //end of namespace BEEV