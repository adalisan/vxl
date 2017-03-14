/* Copyright 2010 Brad King
   Distributed under the Boost Software License, Version 1.0.
   (See accompanying file rtvl_license_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt) */
#include <iostream>
#include <string>
#include <rtvl/rtvl_tensor.h>
#include <rtvl/rtvl_vote.h>
#include <rtvl/rtvl_votee.h>
#include <rtvl/rtvl_voter.h>
#include <rtvl/rtvl_weight_smooth.h>
#include <rtvl/rtvl_weight_original.h>

#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_matrix_fixed.h>
#include <vgl/vgl_point_2d.h>
#include <vcl_compiler.h>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vil/vil_load.h>

#include <vul/vul_arg.h>
int main(int argc, char** argv)
{
  vul_arg<std::string> tokens("-tokens", "input image of tokens", "");
  vul_arg<std::string> saliency("-saliency", "output color saliency image", "");
  vul_arg<double> sparse_scale("-sparse_scale", "range of voting field for sparse voting", 7.0);
  vul_arg<double> dense_scale("-dense_scale", "range of voting field for dense voting", 3.0);
  vul_arg<bool> smooth("-smooth", "use smoothed voting field", false);
  vul_arg<unsigned> display_type("-type", "0-fused with color, 1-stick grey, 2-ball grey", 0);
  vul_arg<bool> show_tokens("-show", "insert tokens in output saliency image", false);
  vul_arg_parse(argc, argv);

  std::string token_path = tokens();
  vil_image_view<vxl_byte> view = vil_load(token_path.c_str());
  unsigned ni = view.ni(), nj = view.nj();
  // Use ball voter matrix
  vnl_matrix_fixed<double, 2, 2> voter_matrix;
  voter_matrix(0,0) = 1.0;
  voter_matrix(1,0) = voter_matrix(0,1) = 0.0;
  voter_matrix(1,1) = 1.0;

  std::vector<vnl_vector_fixed<double, 2> > voter_locations;
  for(unsigned j = 0; j<nj; ++j)
    for(unsigned i = 0; i<ni; ++i){
      if(view(i,j)!=0){
        vnl_vector_fixed<double, 2> voter_location(0.0);
        voter_location[0]=static_cast<double>(i);
        voter_location[1]=static_cast<double>(j);
        voter_locations.push_back(voter_location);
      }
    }
  std::vector<vnl_vector_fixed<double, 2> > votee_locations = voter_locations;
  unsigned n_voters = voter_locations.size(), n_votees = votee_locations.size();
  std::vector<vnl_matrix_fixed<double, 2, 2> > votee_matrices(n_votees,vnl_matrix_fixed<double, 2, 2>(0.0));
  std::vector<rtvl_voter<2> > voters;
  for(std::vector<vnl_vector_fixed<double, 2> >::iterator vit =  voter_locations.begin();
      vit != voter_locations.end(); ++vit){
      rtvl_tensor<2> voter_tensor(voter_matrix);
      rtvl_voter<2> voter(*vit, voter_tensor);
      voters.push_back(voter);
  }

  std::vector<rtvl_votee<2> > votees;
  unsigned i = 0;
  for(std::vector<vnl_vector_fixed<double, 2> >::iterator vit = votee_locations.begin();
      vit != votee_locations.end(); ++vit, i++){
    rtvl_votee<2> votee(*vit, votee_matrices[i]);
    votees.push_back(votee);
  }
  double ss = sparse_scale();
  rtvl_weight_original<2> tvwso(ss);
  rtvl_weight_smooth<2> tvwss(ss);

  // vote between tokens
  bool use_smooth = smooth();
  for(unsigned i = 0; i<n_voters; ++i){
     rtvl_voter<2>& voter = voters[i];
    for(unsigned j = 0; j<n_votees; ++j){
      rtvl_votee<2>& votee = votees[j];
      if(voter.location() == votee.location())
        continue;
      if(use_smooth)
        rtvl_vote(voter, votee, tvwss);
      else
        rtvl_vote(voter, votee, tvwso);
    }
  }

  // dense vote
  // replace voters tensors with sparse vote results
  std::vector<rtvl_voter<2> > dense_voters;
  for(unsigned i = 0; i<n_votees; ++i){
    const vnl_matrix_fixed<double, 2, 2>& mat = votee_matrices[i];
    rtvl_tensor<2> voter_tensor(mat);
    rtvl_voter<2> voter(votees[i].location(), voter_tensor);
    dense_voters.push_back(voter);
  }
  // form dense image of votee_matrices
  std::vector<std::vector<vnl_matrix_fixed<double, 2, 2> > > dense_votee_matrices;
  for(unsigned j = 0; j<nj; ++j){
    std::vector<vnl_matrix_fixed<double, 2, 2> > row;
    for(unsigned i = 0; i<ni; ++i)
      row.push_back(vnl_matrix_fixed<double, 2, 2>(0.0));
    dense_votee_matrices.push_back(row);
  }
  std::vector<std::vector<vnl_vector_fixed<double, 2> > > dense_votee_locations;
  for(unsigned j = 0; j<nj; ++j){
    std::vector<vnl_vector_fixed<double, 2> > row;
    for(unsigned i = 0; i<ni; ++i){
      vnl_vector_fixed<double, 2> loc;
      loc[0]=static_cast<double>(i);
      loc[1]=static_cast<double>(j);
      row.push_back(loc);
    }
    dense_votee_locations.push_back(row);
  }
  std::vector<std::vector<rtvl_votee<2> > > dense_votees;
  for(unsigned j = 0; j<nj; ++j){
    std::vector<rtvl_votee<2> > row;
    for(unsigned i = 0; i<ni; ++i){
      rtvl_votee<2> votee(dense_votee_locations[j][i], dense_votee_matrices[j][i]);
      row.push_back(votee);
    }
    dense_votees.push_back(row);
  }
  double ds = dense_scale();
  rtvl_weight_original<2> tvwdo(ds);
  rtvl_weight_smooth<2> tvwds(ds);
  // carry out dense voting
  for(unsigned k = 0; k<dense_voters.size(); ++k){
    rtvl_voter<2>& voter = dense_voters[k];
    for(unsigned j = 0; j<nj; ++j)
      for(unsigned i = 0; i<ni; ++i){
        rtvl_votee<2>& votee = dense_votees[j][i];
        if(votee.location() == voter.location())
          continue;
        if(use_smooth)
          rtvl_vote(voter, votee, tvwds);
        else
          rtvl_vote(voter, votee, tvwdo);
      }
  }
  // form dense tensor image
  bool show = show_tokens();
  unsigned type = display_type();
  std::string saliency_path = saliency();
  if(type ==0){
  vil_image_view<float> tensor(ni,nj,3);
  tensor.fill(0.0f);
    for(unsigned j = 0; j<nj; ++j)
      for(unsigned i = 0; i<ni; ++i){
        vnl_matrix_fixed<double, 2, 2>& mat = dense_votee_matrices[j][i];
        rtvl_tensor<2> votee_tensor(mat);
        float stick = static_cast<float>(votee_tensor.saliency(0));
        float ball = static_cast<float>(votee_tensor.saliency(1));
        tensor(i,j,1)=stick;
        tensor(i,j,0)=ball;
        tensor(i,j,2)=ball;
        if(show&&view(i,j)>0){
          tensor(i,j,1)=10.0f;
          tensor(i,j,0)=10.0f;
          tensor(i,j,2)=10.0f;
        }
      }
    vil_save(tensor, saliency_path.c_str());
  }else{
    vil_image_view<float> tensor(ni,nj);
    tensor.fill(0.0f);
    for(unsigned j = 0; j<nj; ++j)
      for(unsigned i = 0; i<ni; ++i){
        vnl_matrix_fixed<double, 2, 2>& mat = dense_votee_matrices[j][i];
        rtvl_tensor<2> votee_tensor(mat);
        if(type == 1){
        float stick = static_cast<float>(votee_tensor.saliency(0));
        tensor(i,j)=stick;
        }else if(type == 2){
          float ball = static_cast<float>(votee_tensor.saliency(1));
          tensor(i,j)=ball;
        }else{
          std::cout << "unknown type " << type << std::endl;
          return -1;
        }
        if(show&&view(i,j)>0)
          tensor(i,j)=10.0f;
      }
    vil_save(tensor, saliency_path.c_str());
  }
  return 0;
}
