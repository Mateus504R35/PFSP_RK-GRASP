FROM gcc:14-bookworm

WORKDIR /app

COPY PFSP_RK-GRASP/ ./PFSP_RK-GRASP/

WORKDIR /app/PFSP_RK-GRASP/Program

RUN make clean && make

CMD ["./runTest", "../Instances/PFSP-tests-debug.csv", "3", "0"]