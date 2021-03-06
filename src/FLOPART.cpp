/* -*- compile-command: "R CMD INSTALL .." -*- */

#include <vector>
#include <stdio.h>
#include "funPieceListLog.h"
#include <math.h>
#include <R.h> // for Rprintf
#include "FLOPART.h"

int FLOPART
  (int *data_vec, double *weight_vec, int data_count,
   double penalty,
   // the following matrices are for output.
   // cost_mat and intervals_mat store the optimal cost and number of intervals
   // at each time point, for the up and down cost models.
   // end_vec and mean_vec store the best model up to and including the
   // last data point. 
   double *cost_mat, //data_count x 2.
   int *end_vec,//data_count
   double *mean_vec,//data_count
   int *intervals_mat,//data_count x 2
   int *label_starts,
   int *label_ends,
   int *label_types,
   int label_count){ //matrix where rows are (start, end, labelType)
  bool at_beginning = false;
  bool at_end = false;  
  int curr_label_index = 0;
  int curr_label_type = LABEL_UNLABELED; //1 = peakStart, 0 = noPeak, -1 = peakEnd
  double min_log_mean=INFINITY, max_log_mean=-INFINITY;
  for(int data_i=0; data_i<data_count; data_i++){
    
    double log_data = log( (double)(data_vec[data_i]) );
    if(log_data < min_log_mean){
      min_log_mean = log_data;
    }
    if(max_log_mean < log_data){
      max_log_mean = log_data;
    }
  }
  if(min_log_mean == max_log_mean){
    return ERROR_MIN_MAX_SAME;
  }
  std::vector<PiecewisePoissonLossLog> cost_model_mat(data_count * 2);
  PiecewisePoissonLossLog *up_cost, *down_cost;
  PiecewisePoissonLossLog up_cost_prev, down_cost_prev,
  cost_of_change_up, cost_of_change_down;
  int verbose=0;
  double cum_weight_i = 0.0, cum_weight_prev_i = 0.0;
  
  
  for(int data_i=0; data_i<data_count; data_i++){
    
    cum_weight_i += weight_vec[data_i];
    up_cost = &cost_model_mat[data_i];
    down_cost = &cost_model_mat[data_i + data_count];
    at_beginning = false;
    at_end = false;
    //if we haven't checked all the labels 
    if(curr_label_index < label_count){
      if(data_i >= label_starts[curr_label_index] && data_i <= label_ends[curr_label_index]){
        //if >= current start and <= current end
        
        curr_label_type = label_types[curr_label_index];
        
        if(data_i == label_starts[curr_label_index]){
          at_beginning = true;
        }
        if(data_i == label_ends[curr_label_index]){
          at_end = true;
          curr_label_index++;
        }
        
      }
      else{
        curr_label_type = LABEL_UNLABELED;
      }
    }
    else{
      curr_label_type = LABEL_UNLABELED;
    }
    
    //---UP COST CALCULATIONS---
    
    //if in middle of peak end
    if(data_i == 0){
      up_cost->piece_list.emplace_back
      (1.0, -data_vec[0], 0.0,
       min_log_mean, max_log_mean, -1, false);
    }
    
    else if(curr_label_type == LABEL_PEAKEND && !at_beginning && !at_end){
      //Rprintf("UP: in middle of peak end \n");
      //up cost is just previous up cost
      *up_cost = up_cost_prev;
    }
    
    //if in no peaks or beginning of peak start or end of peak end
    else if(curr_label_type == LABEL_NOPEAKS || (curr_label_type == LABEL_PEAKSTART 
                                                   && at_beginning)
              || (curr_label_type == LABEL_PEAKEND && at_end)){
              // Rprintf("UP: in no peaks or beginning of peak start or end of peak end \n");   
              //up cost is infinite
              up_cost->set_infinite();
    }
    
    //if unlabeled or not beginning of peak start or beginning of peak end  
    else if(curr_label_type == LABEL_UNLABELED 
              || (curr_label_type== LABEL_PEAKSTART && !at_beginning) 
              || (curr_label_type == LABEL_PEAKEND && at_beginning)){
              //Rprintf("UP: unlabeled or not beginning of peakStart or beginning of peakEnd \n");              
              //up cost is min of prev_up, min_less(prev+down + penalty)
              cost_of_change_up.set_to_min_less_of(&down_cost_prev, verbose);
      //seg end  
      cost_of_change_up.set_prev_seg_end(data_i-1);
      cost_of_change_up.addPenalty(penalty, cum_weight_prev_i);
      up_cost->set_to_min_env_of(&up_cost_prev, &cost_of_change_up, verbose);
      
    }
    
    //---DOWN COST CALCULATIONS--- 
    if(data_i==0){
      // initialization Cdown_1(m)=gamma_1(m)/w_1
      down_cost->piece_list.emplace_back
      (1.0, -data_vec[0], 0.0,
       min_log_mean, max_log_mean, -1, false);
      
    }
    //if at beginning of peak end or end of peak start, infinite down
    else if((curr_label_type == LABEL_PEAKEND && at_beginning) ||
            (curr_label_type == LABEL_PEAKSTART && at_end)){
      // Rprintf("DOWN: at beginning of peak end of end of peak start \n");
      down_cost->set_infinite();
    }
    
    //if not at beginning of noPeaks or middle of peak start
    else if((curr_label_type == LABEL_NOPEAKS && !at_beginning)
              || (curr_label_type == LABEL_PEAKSTART && !at_beginning && !at_end)){
      // Rprintf("DOWN: at not beginning of no peaks or middle of peak start \n");     
      *down_cost = down_cost_prev;
      
    }
    //if unlabeled or first or no peaks/peakStart or not at beginning of peak end
    else if(curr_label_type == LABEL_UNLABELED 
              || (curr_label_type == LABEL_PEAKSTART && at_beginning)
              || (curr_label_type == LABEL_NOPEAKS && at_beginning)
              || (curr_label_type == LABEL_PEAKEND && !at_beginning)){
              // Rprintf("DOWN: in unlabeled, beginning peak start, beginning no peaks,  or not beginning of peak end! doing change down or beginning no peaks \n");              
              //down cost is min(prev_down, min_more(prev_up + penalty))
              
              cost_of_change_down.set_to_min_more_of(&up_cost_prev, verbose);
      cost_of_change_down.set_prev_seg_end(data_i-1);
      cost_of_change_down.addPenalty(penalty, cum_weight_prev_i);
      down_cost->set_to_min_env_of(&down_cost_prev, &cost_of_change_down, verbose);
      
    } 
    
    up_cost->adjustWeights(cum_weight_prev_i, cum_weight_i, weight_vec,
                           data_i, data_vec);
    down_cost->adjustWeights(cum_weight_prev_i, cum_weight_i, weight_vec,
                             data_i, data_vec);
    
    
    cum_weight_prev_i = cum_weight_i;
    up_cost_prev = *up_cost;
    down_cost_prev = *down_cost;
    Rprintf("\n\n");
    
  }
  
  
  
  // Decoding the cost_model_vec, and writing to the output matrices.
  PiecewisePoissonLossLog *up_or_down_cost;
  MinimizeResult minResult, bestMinResult;
  bestMinResult.cost = INFINITY;
  bestMinResult.log_mean = INFINITY;
  
  
  
  for(int i=0; i<data_count; i++){
    mean_vec[i] = INFINITY;
    end_vec[i] = -2;
  }
  for(int i=0; i<2*data_count; i++){
    up_or_down_cost = &cost_model_mat[i];
    intervals_mat[i] = (up_or_down_cost->piece_list).size();
    minResult.cost = *cost_mat+i;
    up_or_down_cost->Minimize(&minResult);
    cost_mat[i] = minResult.cost;
    
  }
  
  for(int up_or_down = 1; up_or_down <= 2; up_or_down++){
    //if last cost is up
    if(up_or_down==1){
      minResult.prev_seg_offset = data_count; //previous cost down
    }
    else{
      minResult.prev_seg_offset = 0;
    }
    
    up_or_down_cost = &cost_model_mat[data_count*up_or_down - 1];
    up_or_down_cost->Minimize(&minResult);
    if(minResult.cost < bestMinResult.cost){
      bestMinResult = minResult;
    }
  }
  mean_vec[0] = exp(bestMinResult.log_mean);
  end_vec[0] = bestMinResult.prev_seg_end;
  int out_i=1;
  while(0 <= bestMinResult.prev_seg_end){
    up_or_down_cost = &cost_model_mat[bestMinResult.prev_seg_offset + bestMinResult.prev_seg_end];
    if(bestMinResult.prev_log_mean != INFINITY){
      //equality constraint inactive
      bestMinResult.log_mean = bestMinResult.prev_log_mean;
    }
    up_or_down_cost->findMean
      (&bestMinResult);
    mean_vec[out_i] = exp(bestMinResult.log_mean);
    end_vec[out_i] = bestMinResult.prev_seg_end;
    // change prev_seg_offset and out_i for next iteration.
    if(bestMinResult.prev_seg_offset==0){
      //up_cost is actually up
      bestMinResult.prev_seg_offset = data_count;
    }else{
      //up_cost is actually down
      bestMinResult.prev_seg_offset = 0;
    }
    out_i++;
  }
  
  return 0;
}

