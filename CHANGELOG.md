# Changelog

#### 1.1.0 (2019-03-25)

##### Chores

*  bump tinyxml version, directly use provided lic file ([6b3d7f82](git@code.siemens.com:multicore/drace.git/commit/6b3d7f82160c5c0e34abcc95e99024415fa8b5cc))
*  build dummy detector in CI ([0f119b37](git@code.siemens.com:multicore/drace.git/commit/0f119b37fb578188792e23bb1a21087617393349))
*  always check if copy of dlls is necessary ([a7704736](git@code.siemens.com:multicore/drace.git/commit/a77047366e99f47ec9459487eba10281409654ea))
*  describe static analysis and add CII badge ([557a3c09](git@code.siemens.com:multicore/drace.git/commit/557a3c0953e0c06fcc25731153c6c95d1f446e77))
*  enforce git check if version file not found ([1cc65848](git@code.siemens.com:multicore/drace.git/commit/1cc65848d676ec558946519d1dd120a5ad392343))
*  improved git burn-in script ([2e47ef5e](git@code.siemens.com:multicore/drace.git/commit/2e47ef5e4d5a147ec7109d07a95ef4967cb21e1e))

##### Documentation Changes

*  clarify supported dynamorio versions ([adbcc27f](git@code.siemens.com:multicore/drace.git/commit/adbcc27fbfe91986a558471994485c9088c86f35))
*  update github issue url ([438ea99b](git@code.siemens.com:multicore/drace.git/commit/438ea99b223f5ce15f1444ab715f50b72a1e62a3))
*  note on semantic versioning ([1780a398](git@code.siemens.com:multicore/drace.git/commit/1780a398e7f4987941c40efa8c6a33d73bdf6c5a))
*  add SPDX license file ([f97ba5dc](git@code.siemens.com:multicore/drace.git/commit/f97ba5dce9d2e0442d6a2b700208a7f6395ef730))
*  added reference to conv. changelog ([1d8c2269](git@code.siemens.com:multicore/drace.git/commit/1d8c2269d919a483a0b302c98c6bb586d62a54ff))
*  make licensing information SPDX compatible ([c078bb16](git@code.siemens.com:multicore/drace.git/commit/c078bb16415008053785608d082f414185cd5854))
*  updated licensing information ([d03267e4](git@code.siemens.com:multicore/drace.git/commit/d03267e4a271c864be4e869d8fa063c8ab41b2b4))
*  add list of maintainers ([61417cca](git@code.siemens.com:multicore/drace.git/commit/61417cca8eecc525fec49db91d6f325ae8f597b6))
*  how to cite drace ([5b1ab201](git@code.siemens.com:multicore/drace.git/commit/5b1ab201ea36dba6b58b88bf1a64b7e92c171368))

##### New Features

*  move handle specific logic to tsan ([d1f7abe4](git@code.siemens.com:multicore/drace.git/commit/d1f7abe457ff30891eee7b57bb0b15e8aec05d53))
*  cleanup and streamline tsan-drace modifications ([c12fc0e7](git@code.siemens.com:multicore/drace.git/commit/c12fc0e7341cd5ae15555029c870de670fd51d46))
*  add release and debug version of tsan ([2d1cbb5b](git@code.siemens.com:multicore/drace.git/commit/2d1cbb5b198ccff8058fd3be2f3e5b296bcc888b))
*  add option to control race suppressions ([0b6a3e17](git@code.siemens.com:multicore/drace.git/commit/0b6a3e178e4b578e8b4940398e0dfb1c9c8a8d94))

##### Bug Fixes

*  adapt dummy detector to recent if changes ([523e6cae](git@code.siemens.com:multicore/drace.git/commit/523e6cae512751f34c2bc4ef180fb8bacd3fd80b))
*  resolve many shadow mapping issues ([b993025a](git@code.siemens.com:multicore/drace.git/commit/b993025a9ae6f62d9b19e62bf9a55cdcae292fab))
*  resolve memory leak in managed resolver ([9abf7336](git@code.siemens.com:multicore/drace.git/commit/9abf73363269fb5ae7ffc3dd5023af26af86c8af))
*  resolve linking issue in versioning script ([e75bc001](git@code.siemens.com:multicore/drace.git/commit/e75bc0015a0f3915352133a175e8171fba75e748))
*  do not log after logfile is closed ([53a6afa3](git@code.siemens.com:multicore/drace.git/commit/53a6afa34f5a7954bf1f2e151aa69f69786f3a9f))

##### Other Changes

*  fix error in congested spinlock ([3811650c](git@code.siemens.com:multicore/drace.git/commit/3811650cea6d2a4f403dae04444e94d2a3b0026f))

##### Refactors

*  bump version number due to change in detector if ([e7694757](git@code.siemens.com:multicore/drace.git/commit/e76947575b7ba28916f302dab15feef2cce518fa))
*  fix issues reported by cppcheck ([a6e2f4a0](git@code.siemens.com:multicore/drace.git/commit/a6e2f4a07d8bc7454d0a7aa1f1c0406406fa9e09))
*  fix compiler warnings ([c6f3464d](git@code.siemens.com:multicore/drace.git/commit/c6f3464d88db47f01f4975719cfa3196979736e4))
*  fix compiler warnings in msr ([ba482a98](git@code.siemens.com:multicore/drace.git/commit/ba482a98f16ba09f5aab84ef06ef507929431bb7))

##### Code Style Changes

*  re-format code ([e42de5d0](git@code.siemens.com:multicore/drace.git/commit/e42de5d0fe2c5e22fd5b0de5b89d3190f9691c55))
*  re-format file ([57b5d639](git@code.siemens.com:multicore/drace.git/commit/57b5d639d04dab470d6ec9cf4ad5d0f7fcfe2f6b))
*  properly format file ([0647ebf4](git@code.siemens.com:multicore/drace.git/commit/0647ebf426a02748479357ca8b89b57bbc0db0f7))
*  cleanup formatting in drace-tsan ([c019c48a](git@code.siemens.com:multicore/drace.git/commit/c019c48ae2927b67db5b5bd6124273b1069a3b9c))

##### Tests

*  test client against dr 7.0 and 7.1 ([80a9b20b](git@code.siemens.com:multicore/drace.git/commit/80a9b20b59b20661fef3ee17539f3e2e3f6b46a3))
*  added in-depth test of race report ([1e5ecb68](git@code.siemens.com:multicore/drace.git/commit/1e5ecb68fcf0d94da201b06d38659aac2310d371))
*  disable racyAtomics test due to many spurious failures ([39d6ba89](git@code.siemens.com:multicore/drace.git/commit/39d6ba8923483992a9c802238015b05ad2abcf05))
*  re-enabled racy-atomics test ([15aa5522](git@code.siemens.com:multicore/drace.git/commit/15aa5522253a9704c120a73f913845fa5d581a81))
*  improved test by enforcing data-race ([a195f6bf](git@code.siemens.com:multicore/drace.git/commit/a195f6bfff924f05bea25c2a72e2ce2339edd0ef))
*  re-enabled racy-atomics test ([1d0d7cd4](git@code.siemens.com:multicore/drace.git/commit/1d0d7cd4388ab05506b4c496a79c9c9888f85707))
*  output if data-race acutally happened, enforce race ([d1d0ef86](git@code.siemens.com:multicore/drace.git/commit/d1d0ef867f1b45d2c12beb12d4f2307ef0e8d03c))
*  ensure copy of altered detector dlls in build step ([e66d6756](git@code.siemens.com:multicore/drace.git/commit/e66d675629d2110c3d051401c673af7b1d3a8736))
*  access only heap addresses ([a0c4a624](git@code.siemens.com:multicore/drace.git/commit/a0c4a624ff3f9dd5659dcab59715d8d74596f426))
*  fix annotation integration test ([98880589](git@code.siemens.com:multicore/drace.git/commit/98880589b71378e5ba5e9aa1dd072cce11d45413))
*  print cppcheck results on cl ([a11e01e8](git@code.siemens.com:multicore/drace.git/commit/a11e01e884a12bfb7eb03c92da444c52592c817a))
*  split CI ([acd3f784](git@code.siemens.com:multicore/drace.git/commit/acd3f78403e6f5f7210efe14dcab18a9a68e0f80))
*  enable cppcheck in CI ([bb74ddaa](git@code.siemens.com:multicore/drace.git/commit/bb74ddaabfc76d32609c306a57762750b62107df))
*  copy suppressions file to binary dir ([db8ff7e5](git@code.siemens.com:multicore/drace.git/commit/db8ff7e591ff9fcfce2d74472eeab4b91a3d650b))
*  add cppcheck support in cmake project ([a6d408f3](git@code.siemens.com:multicore/drace.git/commit/a6d408f31b2856f5c87168eeba330eb46f8cc469))
*  fix gtest xml ci ([f490f6a9](git@code.siemens.com:multicore/drace.git/commit/f490f6a9f2b76783f1763239e61bfb4aed538abd))
*  upload gtest results as artifact ([788c0627](git@code.siemens.com:multicore/drace.git/commit/788c0627bb01af88078db70c300c29c34b811010))
*  setup local CI ([5f559487](git@code.siemens.com:multicore/drace.git/commit/5f559487abe8121169a07967bb65b9516a1cc726))

#### 1.0.3

* Initial OSS release (see README.md for details)

