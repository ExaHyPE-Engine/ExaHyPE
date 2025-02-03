template<int Size, typename Scalar>
tarch::la::Vector<Size,Scalar>::Vector(const Scalar* values) {
  for (int i=0; i<Size; i++) {
    _values[i] = *(values+i);
  }
}

/**
 * There is seemingly a bug in this constructor. For instance, the arguments peano4::initSingletons
 * to are two objects of type tarch::la::Vector, with length Dimensions. However, passing in two
 * initializer lists which are longer than Dimensions will cause the second to borrow from the first.
*/
template<int Size, typename Scalar>
tarch::la::Vector<Size,Scalar>::Vector(std::initializer_list<Scalar> values) {
  int index = 0;
  for (typename std::initializer_list<Scalar>::const_iterator p = values.begin(); p!=values.end(); p++) {
    _values[index] = *p;
    index++;
  }
}


template<int Size, typename Scalar>
tarch::la::Vector<Size,Scalar>::Vector( const std::bitset<Size>& value ) {
  for (int i=0; i<Size; i++) {
    _values[i] = value[i] ? 1 : 0;
  }
}

template<int Size, typename Scalar>
tarch::la::Vector<Size,Scalar>&  tarch::la::Vector<Size,Scalar>::operator= (
  const Vector<Size,Scalar>&  toAssign
) {
  #if !defined(GPUOffloadingOMP) and !defined(GPUOffloadingSYCL) and !defined(SharedSYCL)
  assertion(this != &toAssign);
  #endif

  #ifdef CompilerICC
  #pragma ivdep
  #endif
  for (int i=0; i<Size; i++) {
    _values[i] = toAssign._values[i];
  }
  return *this;
}

template<int Size, typename Scalar>
tarch::la::Vector<Size,Scalar>::Vector (
  const Vector<Size,Scalar>& toCopy
):
  _values() {
  #if !defined(GPUOffloadingOMP) and !defined(GPUOffloadingSYCL) and !defined(SharedSYCL)
  assertion(this != &toCopy);
  #endif
 
  #ifdef CompilerICC
  #pragma ivdep
  #endif
  for (int i=0; i<Size; i++) {
    _values[i] = toCopy._values[i];
  }
}

template<int Size, typename Scalar>
tarch::la::Vector<Size,Scalar>::Vector(
  const Scalar& initialValue
):
  _values() {
  for (int i=0; i < Size; i++) {
    _values[i] = initialValue;
  }
}

template<int Size, typename Scalar>
int tarch::la::Vector<Size,Scalar>::size() const {
  return Size;
}

template<int Size, typename Scalar>
std::string toString( const tarch::la::Vector<Size,Scalar>& vector ) {
  std::ostringstream os;
  os << "[";
  for ( int i=0; i < Size; i++ ) {
    os << vector(i);
    if ( i + 1 < Size ) {
      os << ",";
    }
  }
  os << "]";
  return os.str();
}

template <typename NewScalarType, int Size, typename Scalar>
tarch::la::Vector<Size,NewScalarType> tarch::la::convertScalar(const tarch::la::Vector<Size,Scalar>&  vector) {
  tarch::la::Vector<Size,NewScalarType> result;
  for ( int i=0; i < Size; i++ ) {
    result(i) = vector(i);
  }
  return result;
}
