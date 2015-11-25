#pragma once

#include "inner_interfaces.h"
#include "DataReader.h"
#include "commandArgUtil.h" // for intargvector
#include "CUDAPageLockedMemAllocator.h"
#include "IMemoryProvider.h"

namespace Microsoft {
    namespace MSR {
        namespace CNTK {

            template<class ElemType>
            class NormalPacker : public reader
            {
            private:
                std::shared_ptr<IMemoryProvider> m_provider;
                std::shared_ptr<ElemType> Allocate(size_t numberOfElements);

                const static size_t m_htkRandomizeAuto = 0;
                const static size_t m_htkRandomizeDisable = (size_t)-1;

                unique_ptr<msra::dbn::minibatchiterator> m_mbiter;
                unique_ptr<msra::dbn::minibatchsource> m_frameSource;
                unique_ptr<msra::dbn::FileEvalSource> m_fileEvalSource;
                unique_ptr<msra::dbn::latticesource> m_lattices;
                map<wstring, msra::lattices::lattice::htkmlfwordsequence> m_latticeMap;

                vector<bool> m_sentenceEnd;
                bool m_truncated;
                bool m_frameMode;
                vector<size_t> m_processedFrame;
                intargvector m_numSeqsPerMBForAllEpochs;
                size_t m_numSeqsPerMB;                  // requested number of parallel sequences
                size_t m_mbNumTimeSteps;                // number of time steps  to fill/filled (note: for frame randomization, this the #frames, and not 1 as later reported)
                vector<size_t> m_numFramesToProcess;    // [seq index] number of frames available (left to return) in each parallel sequence
                vector<size_t> m_switchFrame;           /// TODO: something like the position where a new sequence starts; still supported?
                vector<size_t> m_numValidFrames;        // [seq index] valid #frames in each parallel sequence. Frames (s, t) with t >= m_numValidFrames[s] are NoInput.
                vector<size_t> m_extraSeqsPerMB;
                size_t m_extraNumSeqs;
                bool m_noData;
                bool m_trainOrTest; // if false, in file writing mode
                using LabelType = typename IDataReader<ElemType>::LabelType;
                using LabelIdType = typename IDataReader<ElemType>::LabelIdType;

                bool m_partialMinibatch; // allow partial minibatches?

                std::vector<std::shared_ptr<ElemType>> m_featuresBufferMultiUtt;
                std::vector<size_t> m_featuresBufferAllocatedMultiUtt;
                std::vector<std::shared_ptr<ElemType>> m_labelsBufferMultiUtt;
                std::vector<size_t> m_labelsBufferAllocatedMultiUtt;
                std::vector<size_t> m_featuresStartIndexMultiUtt;
                std::vector<size_t> m_labelsStartIndexMultiUtt;

                unique_ptr<CUDAPageLockedMemAllocator> m_cudaAllocator;
                std::vector<std::shared_ptr<ElemType>> m_featuresBufferMultiIO;
                std::vector<size_t> m_featuresBufferAllocatedMultiIO;
                std::vector<std::shared_ptr<ElemType>> m_labelsBufferMultiIO;
                std::vector<size_t> m_labelsBufferAllocatedMultiIO;

                //for lattice uids and phoneboundaries
                std::vector<shared_ptr<const msra::dbn::latticesource::latticepair>>  m_latticeBufferMultiUtt;
                std::vector<std::vector<size_t>> m_labelsIDBufferMultiUtt;
                std::vector<std::vector<size_t>> m_phoneboundaryIDBufferMultiUtt;
                std::vector<shared_ptr<const msra::dbn::latticesource::latticepair>>  m_extraLatticeBufferMultiUtt;
                std::vector<std::vector<size_t>> m_extraLabelsIDBufferMultiUtt;
                std::vector<std::vector<size_t>> m_extraPhoneboundaryIDBufferMultiUtt;

                //hmm 
                msra::asr::simplesenonehmm m_hset;

                std::map<std::wstring, size_t> m_featureNameToIdMap;
                std::map<std::wstring, size_t> m_labelNameToIdMap;
                std::map<std::wstring, size_t> m_nameToTypeMap;
                std::map<std::wstring, size_t> m_featureNameToDimMap;
                std::map<std::wstring, size_t> m_labelNameToDimMap;

                // for writing outputs to files (standard single input/output network) - deprecate eventually
                bool m_checkDictionaryKeys;
                bool m_convertLabelsToTargets;
                std::vector <bool> m_convertLabelsToTargetsMultiIO;
                std::vector<std::vector<std::wstring>> m_inputFilesMultiIO;

                size_t m_inputFileIndex;
                std::vector<size_t> m_featDims;
                std::vector<size_t> m_labelDims;

                std::vector<std::vector<std::vector<ElemType>>>m_labelToTargetMapMultiIO;

                int m_verbosity;

                void PrepareForTrainingOrTesting(const ConfigParameters& config);
                void PrepareForWriting(const ConfigParameters& config);

                bool GetMinibatchToTrainOrTest(std::map<std::wstring, Matrix<ElemType>*>&matrices);
                bool GetMinibatch4SEToTrainOrTest(std::vector<shared_ptr<const msra::dbn::latticesource::latticepair>> & latticeinput, vector<size_t> &uids, vector<size_t> &boundaries, std::vector<size_t> &extrauttmap);
                void fillOneUttDataforParallelmode(std::map<std::wstring, Matrix<ElemType>*>& matrices, size_t startFr, size_t framenum, size_t channelIndex, size_t sourceChannelIndex);
                bool GetMinibatchToWrite(std::map<std::wstring, Matrix<ElemType>*>&matrices);

                void StartMinibatchLoopToTrainOrTest(size_t mbSize, size_t epoch, size_t subsetNum, size_t numSubsets, size_t requestedEpochSamples = requestDataSize);
                void StartMinibatchLoopToWrite(size_t mbSize, size_t epoch, size_t requestedEpochSamples = requestDataSize);

                bool ReNewBufferForMultiIO(size_t i);

                size_t GetNumParallelSequences();
                void SetNumParallelSequences(const size_t) { };

                void GetDataNamesFromConfig(const ConfigParameters& readerConfig, std::vector<std::wstring>& features, std::vector<std::wstring>& labels,
                    std::vector<std::wstring>& hmms, std::vector<std::wstring>& lattices);

                size_t ReadLabelToTargetMappingFile(const std::wstring& labelToTargetMappingFile, const std::wstring& labelListFile, std::vector<std::vector<ElemType>>& labelToTargetMap);

                void ExpandDotDotDot(wstring & featPath, const wstring & scpPath, wstring & scpDirCached);


                enum InputOutputTypes
                {
                    real,
                    category,
                };

            public:
                MBLayoutPtr m_pMBLayout;

                /// by default it is false
                /// if true, reader will set to ((int) MinibatchPackingFlags::None) for time positions that are orignally correspond to ((int) MinibatchPackingFlags::SequenceStart)
                /// set to true so that a current minibatch can uses state activities from the previous minibatch. 
                /// default will have truncated BPTT, which only does BPTT inside a minibatch
                bool mIgnoreSentenceBeginTag;
                // TODO: this ^^ does not seem to belong here.

                NormalPacker();

                virtual void Init(const ConfigParameters& config, shared_ptr<IMemoryProvider> memoryProvider);
                virtual void Destroy() { delete this; }
                virtual ~NormalPacker() { }

                virtual void StartMinibatchLoop(size_t mbSize, size_t epoch, size_t requestedEpochSamples = requestDataSize)
                {
                    return StartDistributedMinibatchLoop(mbSize, epoch, 0, 1, requestedEpochSamples);
                }

                virtual bool SupportsDistributedMBRead() const
                {
                    return m_frameSource && m_frameSource->supportsbatchsubsetting();
                }

                virtual void StartDistributedMinibatchLoop(size_t mbSize, size_t epoch, size_t subsetNum, size_t numSubsets, size_t requestedEpochSamples = requestDataSize);

                virtual bool GetMinibatch(std::map<std::wstring, Matrix<ElemType>*>& matrices, MBLayoutPtr returnLayout);
                virtual bool GetMinibatch4SE(std::vector<shared_ptr<const msra::dbn::latticesource::latticepair>> & latticeinput, vector<size_t> &uids, vector<size_t> &boundaries, vector<size_t> &extrauttmap);
                virtual bool GetHmmData(msra::asr::simplesenonehmm * hmm);

                void CopyMBLayoutTo(MBLayoutPtr);
                void SetSentenceEndInBatch(vector<size_t> &/*sentenceEnd*/);
                void SetSentenceEnd(int /*actualMbSize*/){};
                void SetRandomSeed(int){ NOT_IMPLEMENTED };

                virtual std::vector<input_description_ptr> get_inputs() override
                {
                    std::vector<input_description_ptr> result;
                    input_description_ptr features(new input_description);
                    features->name = "features";
                    features->id = 0;
                    result.push_back(features);

                    input_description_ptr labels(new input_description);
                    features->name = "lables";
                    features->id = 1;
                    result.push_back(labels);

                    return result;
                }

                virtual epoch_ptr start_next_epoch(const epoch_configuration& config) override
                {
                    this->StartDistributedMinibatchLoop(config.minibatch_size, 0, config.worker_rank, config.number_of_workers, config.total_size);
                    return epoch_ptr(new epoch_impl(this));
                }

            private:
                class epoch_impl : public epoch
                {
                    NormalPacker<ElemType>* packer_;

                public:
                    epoch_impl(NormalPacker<ElemType>* packer)
                    {
                        packer_ = packer;
                    }

                    virtual minibatch read_minibatch() override;
                };
            };

        }
    }
}