# OS-assignment-4
Group 94

Contribution - 
Dhruv Jaiswal 2023200 - Segmentation fault handling
Akshat Patiyal 2023062 - Error Handling and memory managment

Loader implementation is same as in Assignment 1.
The pagewise allocation takes place at every SEGMENTATION FAULT and reads till the end of the segment or the end of the page, whichever comes first and writes that memory onto the page.
The pages are stored in an array for data cleanup at the end.

https://github.com/CobaltIII/OS-assignment-4
