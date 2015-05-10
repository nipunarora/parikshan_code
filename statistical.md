
INTRODUCTION
------------

Parikshan provides a framework for live debugging. We have seperately used queuing theory for generating an approximate budget for the instrumentation overhead that can be supported in the cloned debugging container.

Here we discuss the potential use cases of statistical debugging in a Parikshan. We take inspiration from the Cooperative Bug Inspection Project from Prof. Ben Liblit at Wisconsin, and from path profiling, and call context encoding mechanisms currently available in the industry.

Our primary goal is to optimize the sampling algorithm to work for a predefined overhead. In particular, we would like to approximate scores given to each of the samples. Based on the same parameter, we would like to give importance to certain predicates over others.

Since we are focused on a fixed budget, we are ok with simply providing a CLUE, i.e. giving up on specificity for speed. Keeping that as a goal our importance is based more on the fact that the predicate should be able to at least localize the problem to a certain section  
