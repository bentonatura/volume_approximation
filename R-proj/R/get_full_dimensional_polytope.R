#' Given a low-dimensional H-polytope, restrict it to its null space 
#' 
#' @param P A low-dimensional convex polytope in H-representation.
#' 
#' @return A list with 4 elements: (a) a full-dimensional polytope in H-representation and (b) the element "N" which is the matrix of the inverse linear transformation that is applied on the input polytope, (c)  the element "shift" which is the opposite vector of that which has shifted the input polytope, (d) the element "svd_prod" which is the product of the singular values of the matrix N and it can be used for further volume estimations.
#'
#' @examples
#' P = gen_cube(10,'H')
#' P$Aeq = gen_rand_hpoly(10,3)$A
#' P$beq = rep(0,3)
#' ret_list = get_full_dimensional_polytope(P)
#' @export
get_full_dimensional_polytope <- function(P){
  
  ret_list = full_dimensional_polytope(P)
  
  #get the matrix that describes the polytope
  Mat = ret_list$Mat
  
  # first column is the vector b
  b = Mat[,1]
  
  # remove first column
  A = Mat[,-c(1)]
  
  PP = list("P" = Hpolytope$new(A,b), "N" = ret_list$N, "shift" = ret_list$shift, "svd_prod" = ret_list$svd_prod)

  return(PP)
}
