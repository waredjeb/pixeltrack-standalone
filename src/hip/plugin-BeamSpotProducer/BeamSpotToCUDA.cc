#include <fstream>

#include <hip/hip_runtime.h>

#include "CUDACore/Product.h"
#include "CUDACore/ScopedContext.h"
#include "CUDACore/copyAsync.h"
#include "CUDACore/host_noncached_unique_ptr.h"
#include "CUDADataFormats/BeamSpotCUDA.h"
#include "DataFormats/BeamSpotPOD.h"
#include "Framework/EDProducer.h"
#include "Framework/Event.h"
#include "Framework/EventSetup.h"
#include "Framework/PluginFactory.h"

class BeamSpotToCUDA : public edm::EDProducer {
public:
  explicit BeamSpotToCUDA(edm::ProductRegistry& reg);
  ~BeamSpotToCUDA() override = default;

  void produce(edm::Event& iEvent, const edm::EventSetup& iSetup) override;

private:
  const edm::EDPutTokenT<cms::hip::Product<BeamSpotCUDA>> bsPutToken_;

  cms::hip::host::noncached::unique_ptr<BeamSpotPOD> bsHost;
};

BeamSpotToCUDA::BeamSpotToCUDA(edm::ProductRegistry& reg)
    : bsPutToken_{reg.produces<cms::hip::Product<BeamSpotCUDA>>()},
      bsHost{cms::hip::make_host_noncached_unique<BeamSpotPOD>(hipHostMallocWriteCombined)} {}

void BeamSpotToCUDA::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
  *bsHost = iSetup.get<BeamSpotPOD>();

  cms::hip::ScopedContextProduce ctx{iEvent.streamID()};

  BeamSpotCUDA bsDevice(ctx.stream());
  cms::hip::copyAsync(bsDevice.ptr(), bsHost, ctx.stream());

  ctx.emplace(iEvent, bsPutToken_, std::move(bsDevice));
}

DEFINE_FWK_MODULE(BeamSpotToCUDA);
