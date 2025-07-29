#ifndef QUALCOMMCAMERA_H
#define QUALCOMMCAMERA_H

#include "CameraAbstraction.h"

namespace evs {
namespace early {
class QualcommCamera : public CameraAbstraction
{
protected:
    int onInit() override;
    void onDeInit() override;
    int onStartPreview() override;
    int onStopPreview() override;

public:
    QualcommCamera() = default;
    ~QualcommCamera() override;

    CameraFrame *getFrame() override final;
    int setConfig(const CameraConfig &config) override;
    CameraConfig getConfig() const override;

private:
};
} // namespace early
} // namespace evs

#endif // QUALCOMMCAMERA_H