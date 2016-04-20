#include "lwtnn/LightweightRNN.hh"


namespace lwt {


  MatrixXd MaskingLayer::scan( const MatrixXd& x) {
    set_mask(   (x.colwise().sum().array() == 0).cast<int>()   );
    return x;
  }

  MatrixXd EmbeddingLayer::scan( const MatrixXd& x) {
    MatrixXd out(_W.rows(), x.cols());
    
    for(int icol=0; icol<x.cols(); icol++)
      out.col(icol) = _W.col( x(0, icol) ) + _b;

    return out;
  }

  MatrixXd TimeDistributedMergeLayer::scan(const MatrixXd& X1, const MatrixXd& X2) {

    if(X1.cols() != X2.cols())
      throw NNEvaluationException("TimeDistributedMergeLayer::scan - Matrices do not have same number of columns (time-dim.)");

    MatrixXd out(X1.rows()+X2.rows(), X1.cols());
    out << X1,
           X2;
    // need to check that this concatenates properly...

    return out;
  }

  LSTMLayer::LSTMLayer(bool return_sequences,
		       std::string activation, std::string inner_activation,
		       MatrixXd W_i, MatrixXd U_i, VectorXd b_i,
		       MatrixXd W_f, MatrixXd U_f, VectorXd b_f,
		       MatrixXd W_o, MatrixXd U_o, VectorXd b_o,
		       MatrixXd W_c, MatrixXd U_c, VectorXd b_c ):
    _return_sequences(return_sequences),
    _activation("tanh"),
    _inner_activation("hard_sigmoid"),
    _W_i(W_i),
    _U_i(U_i),
    _b_i(b_i),
    _W_f(W_f),
    _U_f(U_f),
    _b_f(b_f),
    _W_o(W_o),
    _U_o(U_o),
    _b_o(b_o),
    _W_c(W_c),
    _U_c(U_c),
    _b_c(b_c)
  {
    _n_inputs  = _W_o.cols();
    _n_outputs = _W_o.rows();

    if(_activation=="sigmoid")           _activation_fun = nn_sigmoid;
    else if(_activation=="hard_sigmoid") _activation_fun = nn_hard_sigmoid;
    else if(_activation=="tanh")         _activation_fun = nn_tanh;

    if(_inner_activation=="sigmoid")           _inner_activation_fun = nn_sigmoid;
    else if(_inner_activation=="hard_sigmoid") _inner_activation_fun = nn_hard_sigmoid;
    else if(_inner_activation=="tanh")         _inner_activation_fun = nn_tanh;

  }

  VectorXd LSTMLayer::step( const VectorXd& x_t ) {
    // https://github.com/fchollet/keras/blob/master/keras/layers/recurrent.py#L740

    if(_time < 0)
      throw NNEvaluationException("LSTMLayer::compute - time is less than zero!");

    if(_time == 0){
      if( get_mask()(_time) == 1 ){
	//_C_t.col(_time).setZero();
	//_h_t.col(_time).setZero();
	return VectorXd( _h_t.col(_time) );
      }

      VectorXd i =  (_W_i*x_t + _b_i).unaryExpr(_inner_activation_fun);
      VectorXd f =  (_W_f*x_t + _b_f).unaryExpr(_inner_activation_fun);
      VectorXd o =  (_W_o*x_t + _b_o).unaryExpr(_inner_activation_fun);
      _C_t.col(_time) = i.cwiseProduct(  (_W_c*x_t + _b_c).unaryExpr(_activation_fun)  );
      _h_t.col(_time) = o.cwiseProduct( _C_t.col(_time).unaryExpr(_activation_fun) );
    }

    else{
      if( get_mask()(_time) == 1 ){
	_C_t.col(_time) = _C_t.col(_time - 1);
	_h_t.col(_time) = _h_t.col(_time - 1);
	return VectorXd( _h_t.col(_time) );
      }

      VectorXd h_tm1 = _h_t.col(_time - 1);
      VectorXd C_tm1 = _C_t.col(_time - 1);

      VectorXd i =  (_W_i*x_t + _b_i + _U_i*h_tm1).unaryExpr(_inner_activation_fun);
      VectorXd f =  (_W_f*x_t + _b_f + _U_f*h_tm1).unaryExpr(_inner_activation_fun);
      VectorXd o =  (_W_o*x_t + _b_o + _U_o*h_tm1).unaryExpr(_inner_activation_fun);
      _C_t.col(_time) = f.cwiseProduct(C_tm1) + i.cwiseProduct(  (_W_c*x_t + _b_c + _U_c*h_tm1).unaryExpr(_activation_fun)  );
      _h_t.col(_time) = o.cwiseProduct( _C_t.col(_time).unaryExpr(_activation_fun) );
    }

    return VectorXd( _h_t.col(_time) );
  }

  MatrixXd LSTMLayer::scan( const MatrixXd& x ){

    _C_t.resize(_n_outputs, x.cols());
    _C_t.setZero();
    _h_t.resize(_n_outputs, x.cols());
    _h_t.setZero();
    _time = -1;
    

    for(_time=0; _time < x.cols(); _time++)
      {
	this->step( x.col( _time ) );
      }

    if( this->_return_sequences == true)
      return MatrixXd(_h_t);
  
    return MatrixXd( _h_t.col( _h_t.cols()-1 ) );
  }

}
