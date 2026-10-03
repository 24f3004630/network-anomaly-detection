# LLM Usage Log and Critical Evaluation of AI-Assisted Development

**LLM Tool Used:** Claude (claude.ai)  
**Project:** Hybrid MPI + OpenMP Implementation for Anomaly Detection  
**Development Environment:** Dell Latitude 3440, Ubuntu on WSL2, VS Code, OpenMPI

## 1. LLM Usage Log

The following table documents the prompts used during different stages of the project, how the generated responses contributed to the implementation, and the modifications or verification performed.

| Sr. No. | Purpose | Prompt Used | How the Response Was Used | Modification / Verification |
|---|---|---|---|---|
| 1 | Project planning | **“Analyse the Q33 problem statement and the project instructions I have provided. First, prepare a detailed step-by-step implementation plan covering the project architecture, hybrid MPI + OpenMP parallelization strategy, anomaly detection algorithms, experimental configurations, performance evaluation, repository structure, and report preparation. Ensure that the plan satisfies the assignment requirements and can be completed within the available deadline.”** | Adopted the hybrid MPI + OpenMP architecture, Z-score and K-means detection methods, experimental plan, and repository structure. | Compared the plan against the assignment requirements. Reduced the project scope to meet the deadline, omitted a real-world dataset, and retained Z-score as a sequential implementation. |
| 2 | Hardware-specific adjustments | **“I am going to run this project on my Dell Latitude 3440 laptop. Adapt the implementation and benchmarking plan to my hardware. Explain how to configure WSL2, Ubuntu, OpenMPI, and VS Code; recommend suitable MPI process counts and OpenMP thread counts; and suggest reliable benchmarking practices for this system. Avoid assumptions about my processor specifications and tell me how to verify them.”** | Followed the WSL2, OpenMPI, and VS Code workflow and adopted thread-count experiments from 1 to 12, along with repeated timing measurements. | The initial response incorrectly assumed a 13th-generation CPU. The processor specification was corrected using `lscpu`, which reported an Intel Core i5-1235U. |
| 3 | Development environment and GitHub setup | **“I want to maintain this college project on GitHub and develop it using VS Code. Guide me through the complete setup using VS Code and Ubuntu on WSL2, including repository creation, opening the project in the correct environment, Git initialization, committing files, GitHub authentication, and pushing the code. Explain which terminal I should use for each command and how to troubleshoot common setup failures.”** | Followed the VS Code, WSL, Git, and GitHub workflow for project development and version control. | Several steps failed because commands were executed in the wrong terminal and GitHub authentication was incomplete. The setup was corrected by working inside the Ubuntu terminal and recreating the repository. |
| 4 | Memory constraints and dataset sizing | **“My laptop has limited available memory, and this is a college project that I need to complete within the deadline. Is it necessary to increase the WSL2 memory allocation to 5 GB? Evaluate whether I can work within the existing 3.7 GiB memory limit by reducing the dataset size. Suggest a practical dataset limit and explain how to verify memory and swap usage without unnecessarily modifying my system configuration.”** | Considered the memory requirements and selected an 8-million-row dataset to avoid additional WSL configuration. | Rejected the memory-increase suggestion. The generated dataset was approximately 512 MB and was loaded and processed within the available memory. Memory and swap usage were checked using `free -h`. |
| 5 | Dataset origin and documentation | **“Explain where the dataset used in this project comes from. Inspect the `gen_synthetic.py` script and determine whether the data is downloaded from an external source or generated locally. Explain how the dataset is created, what using synthetic data means for the validity of the experiments, and which limitations should be disclosed in the project report.”** | Used the explanation to document the origin of the dataset and distinguish synthetic data from real-world data. | Inspected `gen_synthetic.py` to verify the dataset's origin. Documented the use of synthetic data as a limitation of the evaluation. |
| 6 | Sequential implementation | **“Implement the sequential baseline for the anomaly detection project. Provide the synthetic dataset generator, a Z-score-based anomaly detector, and a K-means-based anomaly detector using appropriate C and Python code. Ensure that the implementations can be executed and tested independently, produce measurable results, and provide suitable baseline outputs for comparison with the parallel implementation. Explain how precision, recall, false positives, and missed anomalies should be evaluated.”** | Used the generated C and Python code to establish sequential reference implementations and evaluate anomaly detection results. | Executed the implementations. On a 1-million-row dataset, K-means achieved precision 1.0 and recall 1.0; Z-score produced 5 false positives and 0 missed anomalies. |
| 7 | Hybrid MPI + OpenMP implementation | **“Develop the hybrid MPI + OpenMP implementation of the K-means anomaly detection algorithm for this project. Use MPI to distribute the dataset across processes and OpenMP to support shared-memory parallelism within each process. Ensure that the implementation handles uneven data partitions correctly and produces results consistent with the sequential baseline. Explain how to test process counts of 1, 2, 3, and 4 and compare the resulting thresholds, iteration counts, and flagged records.”** | Used the generated hybrid K-means implementation and tested different MPI process counts, including an uneven partition with three processes. | For process counts \(p=1,2,3,4\), verified 20 iterations, a threshold of 11.248, and 10,191 flagged records, matching the sequential reference output. |
| 8 | MPI process placement and performance diagnosis | **“Analyse the timing results for the MPI implementation, particularly the case where the four-process run takes 1.51 seconds while the three-process run takes 1.04 seconds. My Intel Core i5-1235U has a hybrid CPU architecture. Explain possible causes of the timing difference, including process placement, hardware-thread binding, core heterogeneity, and normal measurement variability. Recommend a controlled experiment comparing three placement configurations, with three repetitions per configuration, before concluding which configuration performs best.”** | Used the proposed diagnostic approach to investigate process placement and compare hardware-thread binding configurations. | The first run was not treated as conclusive. Repeated measurements showed approximately 0.82–0.92 seconds for the four-process case, apart from one outlier. Three placement configurations were tested with three runs each. |
| 9 | Benchmark automation | **“Write a complete `bench.sh` script to automate the performance evaluation of my anomaly detection project. The script should execute all 30 planned benchmark configurations five times each, apply the appropriate MPI process-placement options for pure-MPI and hybrid MPI + OpenMP runs, record the relevant configuration parameters and execution times, and save the results in a structured output file. Include checks for missing input data, execution failures, and the actual number of benchmark records generated.”** | Used the script to automate benchmark execution and collect results across configurations and repetitions. | A sanity check identified missing data files. The expected row count was manually recalculated as 30 configurations × 5 repetitions = 150 rows. The script was checked to print the actual number of rows generated. |

**Prompt accuracy note:** The prompts above have been professionally reconstructed from the recorded project activities. They should be treated as polished representations of the requests, not as verbatim historical quotations, except where the original wording is explicitly preserved in the conversation.

---

## 2. Critical Evaluation of LLM Output

### Case 1: WSL2 Memory Limit — Recommendation Rejected

**LLM suggestion:** Increase the WSL2 memory allocation to 5 GB.

**Reason for considering the suggestion:** The environment showed approximately 3.7 GiB of memory, which raised concerns about processing datasets exceeding 16 million rows.

**Decision:** The recommendation was rejected to avoid unnecessary system configuration. The dataset size was capped at 8 million rows instead.

**Verification:** The 8-million-row dataset, approximately 512 MB in size, was successfully loaded and processed within the existing memory limit. Memory and swap usage were checked using `free -h`.

**Evaluation:** The suggestion was reasonable as a possible solution to memory pressure, but increasing the allocation was unnecessary for the final project scope. Reducing the dataset size provided a simpler alternative.

### Case 2: Incorrect Hypothesis About Four-Process Performance

**LLM suggestion:** The slower initial four-process run might have resulted from MPI ranks being assigned to hyper-threads or efficiency cores.

**Reason for considering the suggestion:** The processor uses a hybrid architecture, making process placement a plausible factor affecting performance.

**Outcome:** Repeated runs produced execution times of approximately 0.82–0.92 seconds, with one outlier. This suggested that the original 1.51-second measurement was likely affected by measurement variability rather than a consistently poor placement configuration.

**Verification:** Three placement configurations were evaluated, with three repetitions under each configuration.

**Evaluation:** The initial explanation was a plausible hypothesis, not a confirmed diagnosis. Repeated experiments were necessary to distinguish a persistent performance issue from normal timing variation.

### Case 3: Benchmark Script Binding Options — Recommendation Modified

**LLM suggestion:** Apply the same MPI binding flags, including `--bind-to hwthread`, to all benchmark configurations.

**Problem identified:** Applying hardware-thread binding indiscriminately to hybrid MPI + OpenMP runs could constrain thread placement inappropriately, depending on the MPI mapping and runtime configuration.

**Modification:** Hybrid runs were configured to use `--bind-to none`, while pure-MPI runs retained explicit hardware-thread binding.

**Verification:** The benchmark configurations were separated according to their execution model, with different binding settings for pure MPI and hybrid MPI + OpenMP.

**Evaluation:** The original recommendation did not sufficiently distinguish between the placement requirements of pure-MPI and hybrid execution. The script was modified to account for this difference.

### Case 4: Incorrect Expected Benchmark Row Count — Error Corrected

**LLM suggestion:** The benchmark script was expected to produce approximately 235 rows.

**Problem identified:** The expected output count was calculated incorrectly.

**Correction:**

\[
\text{Expected rows}=\text{Configurations}\times\text{Repetitions}
\]

\[
30\times5=150\text{ rows}
\]

**Verification:** The configuration count was checked manually, and the script was configured to display the actual number of rows produced.

**Evaluation:** This error demonstrated that numerical assumptions in generated scripts must be checked independently. A simple manual calculation established the correct expected output.

---

## 3. Overall Assessment

The LLM assisted with project planning, code generation, environment setup, debugging, and performance evaluation. However, its responses were not accepted without independent scrutiny.

Hardware specifications were verified using system commands, algorithm outputs were compared against the sequential baseline, benchmark timings were repeated, and numerical assumptions were checked manually. Recommendations that were unnecessary, insufficiently supported, or unsuitable for hybrid execution were rejected or modified.

The project demonstrates that **LLMs can accelerate software development, but their outputs must be validated through testing, measurement, and critical reasoning**. The final implementation and report reflect both the assistance received and the independent verification performed by the developer.