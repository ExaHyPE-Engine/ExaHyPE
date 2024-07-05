#pragma once

// This contains loop options for iterating over higher dimensional objects
// they were shamelessly stolen from ExaHyPE1
namespace kernels{

    struct idx2 {
      idx2(int I, int J, int line = -1) : I_(I), J_(J), size(I*J), line_(line) {}

      int operator()(int i, int j) const {
        assertion3(i < I_, i, I_, line_);
        assertion3(j < J_, j, J_, line_);
        return i * J_ + j;
      }

      void rev(int pos, int& i, int& j) const {
        assertion(pos < size);
        i = pos % J_;
        j = pos - i * J_;
      }

      const int I_, J_, size, line_;
    };

    struct idx3 {
    idx3(int I, int J, int K, int line = -1) : I_(I), J_(J), K_(K), size(I*J*K), line_(line) {}

    int operator()(int i, int j, int k)  const {
        assertion3(i < I_, i, I_, line_);
        assertion3(j < J_, j, J_, line_);
        assertion3(k < K_, k, K_, line_);
        return i * (J_ * K_) + j * K_ + k;
    }

    const int I_, J_, K_, size, line_;
    };

    struct idx4 {
    idx4(int I, int J, int K, int L, int line = -1)
        : I_(I), J_(J), K_(K), L_(L), size(I*J*K*L), line_(line) {}

    int operator()(int i, int j, int k, int l) const {
        assertion3(i < I_, i, I_, line_);
        assertion3(j < J_, j, J_, line_);
        assertion3(k < K_, k, K_, line_);
        assertion3(l < L_, l, L_, line_);
        return i * (J_ * K_ * L_) + j * (K_ * L_) + k * L_ + l;
    }

    const int I_, J_, K_, L_, size, line_;
    };

    struct idx5 {
    idx5(int I, int J, int K, int L, int M, int line = -1)
        : I_(I), J_(J), K_(K), L_(L), M_(M), size(I*J*K*L*M), line_(line) {}

    int operator()(int i, int j, int k, int l, int m) const {
        assertion3(i < I_, i, I_, line_);
        assertion3(j < J_, j, J_, line_);
        assertion3(k < K_, k, K_, line_);
        assertion3(l < L_, l, L_, line_);
        assertion3(m < M_, m, M_, line_);
        return i * (J_ * K_ * L_ * M_) + j * (K_ * L_ * M_) + k * (L_ * M_) +
            l * M_ + m;
    }

    const int I_, J_, K_, L_, M_, size, line_;
    };
}