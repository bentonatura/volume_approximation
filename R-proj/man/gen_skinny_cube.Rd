% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/gen_skinny_cube.R
\name{gen_skinny_cube}
\alias{gen_skinny_cube}
\title{Generator function for skinny hypercubes}
\usage{
gen_skinny_cube(dimension)
}
\arguments{
\item{dimension}{The dimension of the skinny hypercube.}
}
\value{
A polytope class representing the \eqn{d}-dimensional skinny hypercube in H-representation.
}
\description{
This function generates a \eqn{d}-dimensional skinny hypercube \eqn{[-1,1]^{d-1}\times [-100,100]}.
}
\examples{
# generate a 10-dimensional skinny hypercube.
P = gen_skinny_cube(10)
}
