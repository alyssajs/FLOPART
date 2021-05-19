# FLOPART 
Functional Labeled Optimal Partitioning - A peak detection algorithm that allows for three types of labels.

# Installation

```r
install.packages("FLOPART")
##OR
if(require("remotes"))install.packages("remotes")
remotes::install_github("alyssajs/FLOPART)
```

# Usage
The main driver function is FLOPART, which takes    
* `data_vec` An integer vector of data   
* `weight_vec` A double vector of weights for each data point  
* `data_count` The number of data points (an integer)
* `penalty` The penalty value (a double)
* `cost_mat` A numeric vector used for output - after execution, the first half of this vector contains the cost of being in the peak state for each data point, and the second half of this 
vector contains the cost of being in the background state for each data point  
* `end_vec` An integer vector of segment ends - finite positive values represent segment endspoints, infinite or negative values are used as placeholders  
* `mean_vec` A double vector of segment means in reverse order  
* `intervals_mat` An integer vector that stores the number of cost function pieces - that is, the number of potential changepoints. Used to analyze time complexity.
* `label_starts` An integer vector of indices where labels start (0-based)  
* `label_ends` An integer vector of indices where labels end (0-based)  
* `label_types` An integer vector of label types where -1 corresponds to a peakEnd label, 0 corresponds to a noPeaks label, and 1 corresponds to a peakStart label  
* `label_count` The number of labels (should match the length of the label_starts, label_ends, and label_types vector) - an integer


FLOPART has an interface function to use FLOPART in R by calling the underlying C++ code.
```r
library(FLOPART)
data_count = 6L
data_vec <- as.integer(c(1,10,10,10,10,1))
weight_vec <- as.double(rep(1, data_count))
penalty <- as.double(0)
cost_mat <- numeric(data_count * 2)
end_vec <- integer(data_count)
mean_vec <- numeric(data_count) 
intervals_mat <- integer(data_count*2)
label_starts <- 1L
label_ends <- 3L
label_types <- 0L
label_count <- 1L

result <- 
  .C("FLOPART_interface", data_vec = data_vec, weight_vec = weight_vec,
     data_count = data_count, penalty = penalty, cost_mat = cost_mat,
   end_vec = end_vec, mean_vec = mean_vec, intervals_mat = intervals_mat,
   label_starts = label_starts, label_ends = label_ends,
   label_types = label_types, label_count = label_count,
   PACKAGE="FLOPART")

costMatrix <- matrix(result[["cost_mat"]], nrow=2, ncol=data_count, byrow=TRUE)
> costMatrix
     [,1]      [,2]      [,3]     [,4]      [,5]      [,6]
[1,]    1       Inf       Inf      Inf -9.100866 -6.621371
[2,]    1 -3.876115 -6.621371 -8.11962 -9.053900 -7.417388
```
Creating the cost matrix from the cost vector in the above way results in a matrix with the entries at [1,i] corresponding to the costs of being in
the peak state for data point i and the entries at [2,i] corresponding to the costs of being in the background