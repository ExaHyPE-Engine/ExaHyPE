// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <iostream>
#include <sstream>
#include <vector>

namespace toolbox {
  namespace curvi {
    class Index {
    public:
      static std::vector<int> getIndexRange(int start, int size) {
        std::vector<int> range;
        for (int i = start; i < start + size; i++) {
          range.push_back(i);
        }
        return range;
      }


      static std::vector<double> getRange(int start, int size, double dx, double ox = 0) {
        std::vector<int>    index_range = getIndexRange(start, size);
        std::vector<double> range;
        for (int i : index_range) {
          range.push_back(i * dx + ox);
        }
        return range;
      }
    };

    class idx2 {
    public:
      idx2(int a_J, int a_I):
        J_(a_J),
        I_(a_I) {
        if (a_J <= 0 || a_I <= 0) {
          std::stringstream ss;
          ss << "Can't initialze index with 0 offset: " << a_J << " " << a_I;
          throw std::domain_error(ss.str());
        }
      };

      int operator()(int j, int i) const {
        if (j < 0 || j > J_) {
          std::stringstream ss;
          ss << "J: " << j << " out of scope : " << J_ << std::endl;
          throw std::domain_error(ss.str());
        }
        return j * I_ + i;
      };

    private:
      int I_;
      int J_;
    };

    class idx3 {
    public:
      idx3(int a_K, int a_J, int a_I):
        K_(a_K),
        J_(a_J),
        I_(a_I) {
        if (a_K <= 0 || a_J <= 0 || a_I <= 0) {
          std::stringstream ss;
          ss << "Can't initialze index with 0 offset: " << a_K << " " << a_J << " " << a_I;
          throw std::domain_error(ss.str());
        }
      };

      int operator()(int k, int j, int i) const {

        if (i < 0 || i > I_) {
          std::stringstream ss;
          ss << "I: " << i << " out of scope : " << I_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (j < 0 || j > J_) {
          std::stringstream ss;
          ss << "J: " << j << " out of scope : " << J_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (k < 0 || k > K_) {
          std::stringstream ss;
          ss << "K: " << k << " out of scope : " << K_ << std::endl;
          throw std::domain_error(ss.str());
        }

        return k * J_ * I_ + j * I_ + i;
      };

    private:
      int I_;
      int J_;
      int K_;
    };

    class idx4 {
    public:
      idx4(int a_L, int a_K, int a_J, int a_I):
        L_(a_L),
        K_(a_K),
        J_(a_J),
        I_(a_I) {};

      int operator()(int l, int k, int j, int i) const {


        if (i < 0 || i > I_) {
          std::stringstream ss;
          ss << "I: " << i << " out of scope : " << I_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (j < 0 || j > J_) {
          std::stringstream ss;
          ss << "J: " << j << " out of scope : " << J_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (k < 0 || k > K_) {
          std::stringstream ss;
          ss << "K: " << k << " out of scope : " << K_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (l < 0 || l > L_) {
          std::stringstream ss;
          ss << "L: " << l << " out of scope : " << L_ << std::endl;
          throw std::domain_error(ss.str());
        }
        return l * K_ * J_ * I_ + k * J_ * I_ + j * I_ + i;
      };

    private:
      int I_;
      int J_;
      int K_;
      int L_;
    };


    class idx6 {
    public:
      idx6(int a_N, int a_M, int a_L, int a_K, int a_J, int a_I):
        N_(a_N),
        M_(a_M),
        L_(a_L),
        K_(a_K),
        J_(a_J),
        I_(a_I) {};

      int operator()(int n, int m, int l, int k, int j, int i) const {

        if (i < 0 || i > I_) {
          std::stringstream ss;
          ss << "I: " << i << " out of scope : " << I_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (j < 0 || j > J_) {
          std::stringstream ss;
          ss << "J: " << j << " out of scope : " << J_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (k < 0 || k > K_) {
          std::stringstream ss;
          ss << "K: " << k << " out of scope : " << K_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (l < 0 || l > L_) {
          std::stringstream ss;
          ss << "L: " << l << " out of scope : " << L_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (m < 0 || m > M_) {
          std::stringstream ss;
          ss << "M: " << m << " out of scope : " << M_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (n < 0 || n > N_) {
          std::stringstream ss;
          ss << "N: " << n << " out of scope : " << N_ << std::endl;
          throw std::domain_error(ss.str());
        }

        return n * M_ * L_ * K_ * J_ * I_ + m * L_ * K_ * J_ * I_ + l * K_ * J_ * I_ + k * J_ * I_ + j * I_ + i;
      };

    private:
      int I_;
      int J_;
      int K_;
      int L_;
      int M_;
      int N_;
    };

    class idx7 {
    public:
      idx7(int a_O, int a_N, int a_M, int a_L, int a_K, int a_J, int a_I):
        O_(a_O),
        N_(a_N),
        M_(a_M),
        L_(a_L),
        K_(a_K),
        J_(a_J),
        I_(a_I) {};

      int operator()(int o, int n, int m, int l, int k, int j, int i) const {

        if (i < 0 || i > I_) {
          std::stringstream ss;
          ss << "I: " << i << " out of scope : " << I_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (j < 0 || j > J_) {
          std::stringstream ss;
          ss << "J: " << j << " out of scope : " << J_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (k < 0 || k > K_) {
          std::stringstream ss;
          ss << "K: " << k << " out of scope : " << K_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (l < 0 || l > L_) {
          std::stringstream ss;
          ss << "L: " << l << " out of scope : " << L_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (m < 0 || m > M_) {
          std::stringstream ss;
          ss << "M: " << m << " out of scope : " << M_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (n < 0 || n > N_) {
          std::stringstream ss;
          ss << "N: " << n << " out of scope : " << N_ << std::endl;
          throw std::domain_error(ss.str());
        }

        if (o < 0 || o > O_) {
          std::stringstream ss;
          ss << "O: " << o << " out of scope : " << O_ << std::endl;
          throw std::domain_error(ss.str());
        }

        return o * N_ * M_ * L_ * K_ * J_ * I_ + n * M_ * L_ * K_ * J_ * I_ + m * L_ * K_ * J_ * I_ + l * K_ * J_ * I_
               + k * J_ * I_ + j * I_ + i;
      };

    private:
      int I_;
      int J_;
      int K_;
      int L_;
      int M_;
      int N_;
      int O_;
    };
  } // namespace curvi
} // namespace toolbox