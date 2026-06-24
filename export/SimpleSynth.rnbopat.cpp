/*******************************************************************************************************************
Copyright (c) 2023 Cycling '74

The code that Max generates automatically and that end users are capable of
exporting and using, and any associated documentation files (the “Software”)
is a work of authorship for which Cycling '74 is the author and owner for
copyright purposes.

This Software is dual-licensed either under the terms of the Cycling '74
License for Max-Generated Code for Export, or alternatively under the terms
of the General Public License (GPL) Version 3. You may use the Software
according to either of these licenses as it is most appropriate for your
project on a case-by-case basis (proprietary or not).

A) Cycling '74 License for Max-Generated Code for Export

A license is hereby granted, free of charge, to any person obtaining a copy
of the Software (“Licensee”) to use, copy, modify, merge, publish, and
distribute copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The Software is licensed to Licensee for all uses that do not include the sale,
sublicensing, or commercial distribution of software that incorporates this
source code. This means that the Licensee is free to use this software for
educational, research, and prototyping purposes, to create musical or other
creative works with software that incorporates this source code, or any other
use that does not constitute selling software that makes use of this source
code. Commercial distribution also includes the packaging of free software with
other paid software, hardware, or software-provided commercial services.

For entities with UNDER $200k in annual revenue or funding, a license is hereby
granted, free of charge, for the sale, sublicensing, or commercial distribution
of software that incorporates this source code, for as long as the entity's
annual revenue remains below $200k annual revenue or funding.

For entities with OVER $200k in annual revenue or funding interested in the
sale, sublicensing, or commercial distribution of software that incorporates
this source code, please send inquiries to licensing@cycling74.com.

The above copyright notice and this license shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Please see
https://support.cycling74.com/hc/en-us/articles/10730637742483-RNBO-Export-Licensing-FAQ
for additional information

B) General Public License Version 3 (GPLv3)
Details of the GPLv3 license can be found at: https://www.gnu.org/licenses/gpl-3.0.html
*******************************************************************************************************************/

#ifdef RNBO_LIB_PREFIX
#define STR_IMPL(A) #A
#define STR(A) STR_IMPL(A)
#define RNBO_LIB_INCLUDE(X) STR(RNBO_LIB_PREFIX/X)
#else
#define RNBO_LIB_INCLUDE(X) #X
#endif // RNBO_LIB_PREFIX
#ifdef RNBO_INJECTPLATFORM
#define RNBO_USECUSTOMPLATFORM
#include RNBO_INJECTPLATFORM
#endif // RNBO_INJECTPLATFORM

#include RNBO_LIB_INCLUDE(RNBO_Common.h)
#include RNBO_LIB_INCLUDE(RNBO_AudioSignal.h)

namespace RNBO {


#define trunc(x) ((Int)(x))
#define autoref auto&

#if defined(__GNUC__) || defined(__clang__)
    #define RNBO_RESTRICT __restrict__
#elif defined(_MSC_VER)
    #define RNBO_RESTRICT __restrict
#endif

#define FIXEDSIZEARRAYINIT(...) { }

template <class ENGINE = INTERNALENGINE> class rnbomatic : public PatcherInterfaceImpl {

friend class EngineCore;
friend class Engine;
friend class MinimalEngine<>;
public:

rnbomatic()
: _internalEngine(this)
{
}

~rnbomatic()
{
    deallocateSignals();
}

Index getNumMidiInputPorts() const {
    return 1;
}

void processMidiEvent(MillisecondTime time, int port, ConstByteArray data, Index length) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->notein_01_midihandler(data[0] & 240, (data[0] & 15) + 1, port, data, length);
}

Index getNumMidiOutputPorts() const {
    return 0;
}

void process(
    const SampleValue * const* inputs,
    Index numInputs,
    SampleValue * const* outputs,
    Index numOutputs,
    Index n
) {
    RNBO_UNUSED(numInputs);
    RNBO_UNUSED(inputs);
    this->vs = n;
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
    SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);

    this->cycle_tilde_01_perform(
        this->cycle_tilde_01_frequency,
        this->cycle_tilde_01_phase_offset,
        this->signals[0],
        this->dummyBuffer,
        n
    );

    this->linetilde_01_perform(this->signals[1], n);
    this->dspexpr_01_perform(this->signals[0], this->signals[1], out1, n);
    this->stackprotect_perform(n);
    this->globaltransport_advance();
    this->advanceTime((ENGINE*)nullptr);
    this->audioProcessSampleCount += this->vs;
}

void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
    RNBO_ASSERT(this->_isInitialized);

    if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
        Index i;

        for (i = 0; i < 2; i++) {
            this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
        }

        this->globaltransport_tempo = resizeSignal(this->globaltransport_tempo, this->maxvs, maxBlockSize);
        this->globaltransport_state = resizeSignal(this->globaltransport_state, this->maxvs, maxBlockSize);
        this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
        this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
        this->didAllocateSignals = true;
    }

    const bool sampleRateChanged = sampleRate != this->sr;
    const bool maxvsChanged = maxBlockSize != this->maxvs;
    const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;

    if (sampleRateChanged || maxvsChanged) {
        this->vs = maxBlockSize;
        this->maxvs = maxBlockSize;
        this->sr = sampleRate;
        this->invsr = 1 / sampleRate;
    }

    this->cycle_tilde_01_dspsetup(forceDSPSetup);
    this->globaltransport_dspsetup(forceDSPSetup);

    if (sampleRateChanged)
        this->onSampleRateChanged(sampleRate);
}

number msToSamps(MillisecondTime ms, number sampleRate) {
    return ms * sampleRate * 0.001;
}

MillisecondTime sampsToMs(SampleIndex samps) {
    return samps * (this->invsr * 1000);
}

Index getNumInputChannels() const {
    return 0;
}

Index getNumOutputChannels() const {
    return 1;
}

DataRef* getDataRef(DataRefIndex index)  {
    switch (index) {
    case 0:
        {
        return addressOf(this->RNBODefaultMtofLookupTable256);
        break;
        }
    case 1:
        {
        return addressOf(this->RNBODefaultSinus);
        break;
        }
    default:
        {
        return nullptr;
        }
    }
}

DataRefIndex getNumDataRefs() const {
    return 2;
}

void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (index == 0) {
        this->mtof_01_innerMtoF_buffer = reInitDataView(this->mtof_01_innerMtoF_buffer, this->RNBODefaultMtofLookupTable256);
    }

    if (index == 1) {
        this->cycle_tilde_01_buffer = reInitDataView(this->cycle_tilde_01_buffer, this->RNBODefaultSinus);
        this->cycle_tilde_01_bufferUpdated();
    }
}

void initialize() {
    RNBO_ASSERT(!this->_isInitialized);

    this->RNBODefaultMtofLookupTable256 = initDataRef(
        this->RNBODefaultMtofLookupTable256,
        this->dataRefStrings->name0,
        true,
        this->dataRefStrings->file0,
        this->dataRefStrings->tag0
    );

    this->RNBODefaultSinus = initDataRef(
        this->RNBODefaultSinus,
        this->dataRefStrings->name1,
        true,
        this->dataRefStrings->file1,
        this->dataRefStrings->tag1
    );

    this->assign_defaults();
    this->applyState();
    this->RNBODefaultMtofLookupTable256->setIndex(0);
    this->mtof_01_innerMtoF_buffer = new SampleBuffer(this->RNBODefaultMtofLookupTable256);
    this->RNBODefaultSinus->setIndex(1);
    this->cycle_tilde_01_buffer = new SampleBuffer(this->RNBODefaultSinus);
    this->initializeObjects();
    this->allocateDataRefs();
    this->startup();
    this->_isInitialized = true;
}

void getPreset(PatcherStateInterface& preset) {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
    preset["__presetid"] = "rnbo";
}

void setPreset(MillisecondTime time, PatcherStateInterface& preset) {
    this->updateTime(time, (ENGINE*)nullptr);
}

void setParameterValue(ParameterIndex , ParameterValue , MillisecondTime ) {}

void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValue(index, value, time);
}

void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
    this->setParameterValue(index, this->getParameterValue(index), time);
}

void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValueNormalized(index, value, time);
}

ParameterValue getParameterValue(ParameterIndex index)  {
    switch (index) {
    default:
        {
        return 0;
        }
    }
}

ParameterIndex getNumSignalInParameters() const {
    return 0;
}

ParameterIndex getNumSignalOutParameters() const {
    return 0;
}

ParameterIndex getNumParameters() const {
    return 0;
}

ConstCharPointer getParameterName(ParameterIndex index) const {
    switch (index) {
    default:
        {
        return "bogus";
        }
    }
}

ConstCharPointer getParameterId(ParameterIndex index) const {
    switch (index) {
    default:
        {
        return "bogus";
        }
    }
}

void getParameterInfo(ParameterIndex , ParameterInfo * ) const {}

ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
    if (steps == 1) {
        if (normalizedValue > 0) {
            normalizedValue = 1.;
        }
    } else {
        ParameterValue oneStep = (number)1. / (steps - 1);
        ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
        normalizedValue = numberOfSteps * oneStep;
    }

    return normalizedValue;
}

ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    default:
        {
        return value;
        }
    }
}

ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

    switch (index) {
    default:
        {
        return value;
        }
    }
}

ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    default:
        {
        return value;
        }
    }
}

void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("listin"):
        {
        if (TAG("message_obj-5") == objectId)
            this->message_01_listin_number_set(payload);

        break;
        }
    }
}

void processListMessage(
    MessageTag tag,
    MessageTag objectId,
    MillisecondTime time,
    const list& payload
) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("listin"):
        {
        if (TAG("message_obj-5") == objectId)
            this->message_01_listin_list_set(payload);

        break;
        }
    }
}

void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("listin"):
        {
        if (TAG("message_obj-5") == objectId)
            this->message_01_listin_bang_bang();

        break;
        }
    }
}

MessageTagInfo resolveTag(MessageTag tag) const {
    switch (tag) {
    case TAG("listout"):
        {
        return "listout";
        }
    case TAG("message_obj-5"):
        {
        return "message_obj-5";
        }
    case TAG("listin"):
        {
        return "listin";
        }
    }

    return "";
}

MessageIndex getNumMessages() const {
    return 0;
}

const MessageInfo& getMessageInfo(MessageIndex index) const {
    switch (index) {

    }

    return NullMessageInfo;
}

protected:

		
void advanceTime(EXTERNALENGINE*) {}
void advanceTime(INTERNALENGINE*) {
	_internalEngine.advanceTime(sampstoms(this->vs));
}

void processInternalEvents(MillisecondTime time) {
	_internalEngine.processEventsUntil(time);
}

void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
	if (time == TimeNow) time = getPatcherTime();
	processInternalEvents(inProcess ? time + sampsToMs(this->vs) : time);
	updateTime(time, (EXTERNALENGINE*)nullptr);
}

rnbomatic* operator->() {
    return this;
}
const rnbomatic* operator->() const {
    return this;
}
rnbomatic* getTopLevelPatcher() {
    return this;
}

void cancelClockEvents()
{
    getEngine()->flushClockEvents(this, -281953904, false);
}

template<typename LISTTYPE = list> void listquicksort(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    if (l < h) {
        Int p = (Int)(this->listpartition(arr, sortindices, l, h, ascending));
        this->listquicksort(arr, sortindices, l, p - 1, ascending);
        this->listquicksort(arr, sortindices, p + 1, h, ascending);
    }
}

template<typename LISTTYPE = list> Int listpartition(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    number x = arr[(Index)h];
    Int i = (Int)(l - 1);

    for (Int j = (Int)(l); j <= h - 1; j++) {
        bool asc = (bool)((bool)(ascending) && arr[(Index)j] <= x);
        bool desc = (bool)((bool)(!(bool)(ascending)) && arr[(Index)j] >= x);

        if ((bool)(asc) || (bool)(desc)) {
            i++;
            this->listswapelements(arr, i, j);
            this->listswapelements(sortindices, i, j);
        }
    }

    i++;
    this->listswapelements(arr, i, h);
    this->listswapelements(sortindices, i, h);
    return i;
}

template<typename LISTTYPE = list> void listswapelements(LISTTYPE& arr, Int a, Int b) {
    auto tmp = arr[(Index)a];
    arr[(Index)a] = arr[(Index)b];
    arr[(Index)b] = tmp;
}

inline number linearinterp(number frac, number x, number y) {
    return x + (y - x) * frac;
}

UInt64 currentsampletime() {
    return this->audioProcessSampleCount + this->sampleOffsetIntoNextAudioBuffer;
}

number mstosamps(MillisecondTime ms) {
    return ms * this->sr * 0.001;
}

number maximum(number x, number y) {
    return (x < y ? y : x);
}

MillisecondTime sampstoms(number samps) {
    return samps * 1000 / this->sr;
}

MillisecondTime getPatcherTime() const {
    return this->_currentTime;
}

template<typename LISTTYPE> void message_01_listin_list_set(const LISTTYPE& v) {
    this->message_01_set_set(v);
}

void message_01_listin_number_set(number v) {
    this->message_01_set_set(v);
}

void message_01_listin_bang_bang() {
    this->message_01_trigger_bang();
}

void linetilde_01_target_bang() {}

void deallocateSignals() {
    Index i;

    for (i = 0; i < 2; i++) {
        this->signals[i] = freeSignal(this->signals[i]);
    }

    this->globaltransport_tempo = freeSignal(this->globaltransport_tempo);
    this->globaltransport_state = freeSignal(this->globaltransport_state);
    this->zeroBuffer = freeSignal(this->zeroBuffer);
    this->dummyBuffer = freeSignal(this->dummyBuffer);
}

Index getMaxBlockSize() const {
    return this->maxvs;
}

number getSampleRate() const {
    return this->sr;
}

bool hasFixedVectorSize() const {
    return false;
}

void setProbingTarget(MessageTag ) {}

void fillRNBODefaultMtofLookupTable256(DataRef& ref) {
    SampleBuffer buffer(ref);
    number bufsize = buffer->getSize();

    for (Index i = 0; i < bufsize; i++) {
        number midivalue = -256. + (number)512. / (bufsize - 1) * i;
        buffer[i] = rnbo_exp(.057762265 * (midivalue - 69.0));
    }
}

void fillRNBODefaultSinus(DataRef& ref) {
    SampleBuffer buffer(ref);
    number bufsize = buffer->getSize();

    for (Index i = 0; i < bufsize; i++) {
        buffer[i] = rnbo_cos(i * 3.14159265358979323846 * 2. / bufsize);
    }
}

void fillDataRef(DataRefIndex index, DataRef& ref) {
    switch (index) {
    case 0:
        {
        this->fillRNBODefaultMtofLookupTable256(ref);
        break;
        }
    case 1:
        {
        this->fillRNBODefaultSinus(ref);
        break;
        }
    }
}

void allocateDataRefs() {
    this->mtof_01_innerMtoF_buffer->requestSize(65536, 1);
    this->mtof_01_innerMtoF_buffer->setSampleRate(this->sr);
    this->cycle_tilde_01_buffer->requestSize(16384, 1);
    this->cycle_tilde_01_buffer->setSampleRate(this->sr);
    this->mtof_01_innerMtoF_buffer = this->mtof_01_innerMtoF_buffer->allocateIfNeeded();

    if (this->RNBODefaultMtofLookupTable256->hasRequestedSize()) {
        if (this->RNBODefaultMtofLookupTable256->wantsFill())
            this->fillRNBODefaultMtofLookupTable256(this->RNBODefaultMtofLookupTable256);

        this->getEngine()->sendDataRefUpdated(0);
    }

    this->cycle_tilde_01_buffer = this->cycle_tilde_01_buffer->allocateIfNeeded();

    if (this->RNBODefaultSinus->hasRequestedSize()) {
        if (this->RNBODefaultSinus->wantsFill())
            this->fillRNBODefaultSinus(this->RNBODefaultSinus);

        this->getEngine()->sendDataRefUpdated(1);
    }
}

void initializeObjects() {
    this->mtof_01_innerScala_init();
    this->mtof_01_init();
    this->message_01_init();
}

Index getIsMuted()  {
    return this->isMuted;
}

void setIsMuted(Index v)  {
    this->isMuted = v;
}

void onSampleRateChanged(double ) {}

void extractState(PatcherStateInterface& ) {}

void applyState() {}

void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
    RNBO_UNUSED(value);
    RNBO_UNUSED(hasValue);
    this->updateTime(time, (ENGINE*)nullptr);

    switch (index) {
    case -281953904:
        {
        this->linetilde_01_target_bang();
        break;
        }
    }
}

void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}

void processOutletEvent(
    EngineLink* sender,
    OutletIndex index,
    ParameterValue value,
    MillisecondTime time
) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->processOutletAtCurrentTime(sender, index, value);
}

void sendOutlet(OutletIndex index, ParameterValue value) {
    this->getEngine()->sendOutlet(this, index, value);
}

void startup() {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
    this->processParamInitEvents();
}

template<typename LISTTYPE> void message_01_set_set(const LISTTYPE& v) {
    this->message_01_set = jsCreateListCopy(v);
}

void linetilde_01_time_set(number v) {
    this->linetilde_01_time = v;
}

template<typename LISTTYPE> void linetilde_01_segments_set(const LISTTYPE& v) {
    this->linetilde_01_segments = jsCreateListCopy(v);

    if ((bool)(v->length)) {
        if (v->length == 1 && this->linetilde_01_time == 0) {
            this->linetilde_01_activeRamps->length = 0;
            this->linetilde_01_currentValue = v[0];
        } else {
            auto currentTime = this->currentsampletime();
            number lastRampValue = this->linetilde_01_currentValue;
            number rampEnd = currentTime - this->sampleOffsetIntoNextAudioBuffer;

            for (Index i = 0; i < this->linetilde_01_activeRamps->length; i += 3) {
                rampEnd = this->linetilde_01_activeRamps[(Index)(i + 2)];

                if (rampEnd > currentTime) {
                    this->linetilde_01_activeRamps[(Index)(i + 2)] = currentTime;
                    number diff = rampEnd - currentTime;
                    number valueDiff = diff * this->linetilde_01_activeRamps[(Index)(i + 1)];
                    lastRampValue = this->linetilde_01_activeRamps[(Index)i] - valueDiff;
                    this->linetilde_01_activeRamps[(Index)i] = lastRampValue;
                    this->linetilde_01_activeRamps->length = i + 3;
                    rampEnd = currentTime;
                } else {
                    lastRampValue = this->linetilde_01_activeRamps[(Index)i];
                }
            }

            if (rampEnd < currentTime) {
                this->linetilde_01_activeRamps->push(lastRampValue);
                this->linetilde_01_activeRamps->push(0);
                this->linetilde_01_activeRamps->push(currentTime);
            }

            number lastRampEnd = currentTime;

            for (Index i = 0; i < v->length; i += 2) {
                number destinationValue = v[(Index)i];
                number inc = 0;
                number rampTimeInSamples;

                if (v->length > i + 1) {
                    rampTimeInSamples = this->mstosamps(v[(Index)(i + 1)]);

                    if ((bool)(this->linetilde_01_keepramp)) {
                        this->linetilde_01_time_set(v[(Index)(i + 1)]);
                    }
                } else {
                    rampTimeInSamples = this->mstosamps(this->linetilde_01_time);
                }

                if (rampTimeInSamples <= 0) {
                    rampTimeInSamples = 1;
                }

                inc = (destinationValue - lastRampValue) / rampTimeInSamples;
                lastRampEnd += rampTimeInSamples;
                this->linetilde_01_activeRamps->push(destinationValue);
                this->linetilde_01_activeRamps->push(inc);
                this->linetilde_01_activeRamps->push(lastRampEnd);
                lastRampValue = destinationValue;
            }
        }
    }
}

template<typename LISTTYPE> void message_01_out_set(const LISTTYPE& v) {
    this->linetilde_01_segments_set(v);
}

void message_01_trigger_bang() {
    if ((bool)(this->message_01_set->length) || (bool)(false)) {
        this->message_01_out_set(this->message_01_set);
    }
}

void notein_01_outchannel_set(number ) {}

void notein_01_releasevelocity_set(number ) {}

void select_01_match1_bang() {}

void select_01_nomatch_number_set(number v) {
    RNBO_UNUSED(v);
    this->message_01_trigger_bang();
}

void select_01_input_number_set(number v) {
    if (v == this->select_01_test1)
        this->select_01_match1_bang();
    else
        this->select_01_nomatch_number_set(v);
}

void notein_01_velocity_set(number v) {
    this->select_01_input_number_set(v);
}

void cycle_tilde_01_frequency_set(number v) {
    this->cycle_tilde_01_frequency = v;
}

void cycle_tilde_01_phase_offset_set(number v) {
    this->cycle_tilde_01_phase_offset = v;
}

template<typename LISTTYPE> void mtof_01_out_set(const LISTTYPE& v) {
    {
        if (v->length > 1)
            this->cycle_tilde_01_phase_offset_set(v[1]);

        number converted = (v->length > 0 ? v[0] : 0);
        this->cycle_tilde_01_frequency_set(converted);
    }
}

template<typename LISTTYPE> void mtof_01_midivalue_set(const LISTTYPE& v) {
    this->mtof_01_midivalue = jsCreateListCopy(v);
    list tmp = list();

    for (Int i = 0; i < this->mtof_01_midivalue->length; i++) {
        tmp->push(
            this->mtof_01_innerMtoF_next(this->mtof_01_midivalue[(Index)i], this->mtof_01_base)
        );
    }

    this->mtof_01_out_set(tmp);
}

void notein_01_notenumber_set(number v) {
    {
        listbase<number, 1> converted = {v};
        this->mtof_01_midivalue_set(converted);
    }
}

void notein_01_midihandler(int status, int channel, int port, ConstByteArray data, Index length) {
    RNBO_UNUSED(length);
    RNBO_UNUSED(port);

    if (channel == this->notein_01_channel || this->notein_01_channel <= 0) {
        if (status == 144 || status == 128) {
            this->notein_01_outchannel_set(channel);

            if (status == 128) {
                this->notein_01_releasevelocity_set(data[2]);
                this->notein_01_velocity_set(0);
            } else {
                this->notein_01_releasevelocity_set(0);
                this->notein_01_velocity_set(data[2]);
            }

            this->notein_01_notenumber_set(data[1]);
        }
    }
}

void cycle_tilde_01_perform(
    number frequency,
    number phase_offset,
    SampleValue * out1,
    SampleValue * out2,
    Index n
) {
    RNBO_UNUSED(phase_offset);
    auto __cycle_tilde_01_f2i = this->cycle_tilde_01_f2i;
    auto __cycle_tilde_01_buffer = this->cycle_tilde_01_buffer;
    auto __cycle_tilde_01_phasei = this->cycle_tilde_01_phasei;
    Index i;

    for (i = 0; i < (Index)n; i++) {
        {
            UInt32 uint_phase;

            {
                {
                    uint_phase = __cycle_tilde_01_phasei;
                }
            }

            UInt32 idx = (UInt32)(uint32_rshift(uint_phase, 18));
            number frac = ((BinOpInt)((BinOpInt)uint_phase & (BinOpInt)262143)) * 3.81471181759574e-6;
            number y0 = __cycle_tilde_01_buffer[(Index)idx];
            number y1 = __cycle_tilde_01_buffer[(Index)((BinOpInt)(idx + 1) & (BinOpInt)16383)];
            number y = y0 + frac * (y1 - y0);

            {
                UInt32 pincr = (UInt32)(uint32_trunc(frequency * __cycle_tilde_01_f2i));
                __cycle_tilde_01_phasei = uint32_add(__cycle_tilde_01_phasei, pincr);
            }

            out1[(Index)i] = y;
            out2[(Index)i] = uint_phase * 0.232830643653869629e-9;
            continue;
        }
    }

    this->cycle_tilde_01_phasei = __cycle_tilde_01_phasei;
}

void linetilde_01_perform(SampleValue * out, Index n) {
    auto __linetilde_01_time = this->linetilde_01_time;
    auto __linetilde_01_keepramp = this->linetilde_01_keepramp;
    auto __linetilde_01_currentValue = this->linetilde_01_currentValue;
    Index i = 0;

    if ((bool)(this->linetilde_01_activeRamps->length)) {
        while ((bool)(this->linetilde_01_activeRamps->length) && i < n) {
            number destinationValue = this->linetilde_01_activeRamps[0];
            number inc = this->linetilde_01_activeRamps[1];
            number rampTimeInSamples = this->linetilde_01_activeRamps[2] - this->audioProcessSampleCount - i;
            number val = __linetilde_01_currentValue;

            while (rampTimeInSamples > 0 && i < n) {
                out[(Index)i] = val;
                val += inc;
                i++;
                rampTimeInSamples--;
            }

            if (rampTimeInSamples <= 0) {
                val = destinationValue;
                this->linetilde_01_activeRamps->splice(0, 3);

                if ((bool)(!(bool)(this->linetilde_01_activeRamps->length))) {
                    this->getEngine()->scheduleClockEventWithValue(
                        this,
                        -281953904,
                        this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                        0
                    );;

                    if ((bool)(!(bool)(__linetilde_01_keepramp))) {
                        __linetilde_01_time = 0;
                    }
                }
            }

            __linetilde_01_currentValue = val;
        }
    }

    while (i < n) {
        out[(Index)i] = __linetilde_01_currentValue;
        i++;
    }

    this->linetilde_01_currentValue = __linetilde_01_currentValue;
    this->linetilde_01_time = __linetilde_01_time;
}

void dspexpr_01_perform(const Sample * in1, const Sample * in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2[(Index)i];//#map:_###_obj_###_:1
    }
}

void stackprotect_perform(Index n) {
    RNBO_UNUSED(n);
    auto __stackprotect_count = this->stackprotect_count;
    __stackprotect_count = 0;
    this->stackprotect_count = __stackprotect_count;
}

number mtof_01_innerMtoF_next(number midivalue, number tuning) {
    if (midivalue == this->mtof_01_innerMtoF_lastInValue && tuning == this->mtof_01_innerMtoF_lastTuning)
        return this->mtof_01_innerMtoF_lastOutValue;

    this->mtof_01_innerMtoF_lastInValue = midivalue;
    this->mtof_01_innerMtoF_lastTuning = tuning;
    number result = 0;

    {
        result = rnbo_exp(.057762265 * (midivalue - 69.0));
    }

    this->mtof_01_innerMtoF_lastOutValue = tuning * result;
    return this->mtof_01_innerMtoF_lastOutValue;
}

void mtof_01_innerMtoF_reset() {
    this->mtof_01_innerMtoF_lastInValue = 0;
    this->mtof_01_innerMtoF_lastOutValue = 0;
    this->mtof_01_innerMtoF_lastTuning = 0;
}

void mtof_01_innerScala_mid(Int v) {
    this->mtof_01_innerScala_kbmMid = v;
    this->mtof_01_innerScala_updateRefFreq();
}

void mtof_01_innerScala_ref(Int v) {
    this->mtof_01_innerScala_kbmRefNum = v;
    this->mtof_01_innerScala_updateRefFreq();
}

void mtof_01_innerScala_base(number v) {
    this->mtof_01_innerScala_kbmRefFreq = v;
    this->mtof_01_innerScala_updateRefFreq();
}

void mtof_01_innerScala_init() {
    list sclValid = {
        12,
        100,
        0,
        200,
        0,
        300,
        0,
        400,
        0,
        500,
        0,
        600,
        0,
        700,
        0,
        800,
        0,
        900,
        0,
        1000,
        0,
        1100,
        0,
        2,
        1
    };

    this->mtof_01_innerScala_updateScale(sclValid);
}

template<typename LISTTYPE = list> void mtof_01_innerScala_update(const LISTTYPE& scale, const LISTTYPE& map) {
    if (scale->length > 0) {
        this->mtof_01_innerScala_updateScale(scale);
    }

    if (map->length > 0) {
        this->mtof_01_innerScala_updateMap(map);
    }
}

number mtof_01_innerScala_mtof(number note) {
    if ((bool)(this->mtof_01_innerScala_lastValid) && this->mtof_01_innerScala_lastNote == note) {
        return this->mtof_01_innerScala_lastFreq;
    }

    array<Int, 2> degoct = this->mtof_01_innerScala_applyKBM(note);
    number out = 0;

    if (degoct[1] > 0) {
        out = this->mtof_01_innerScala_applySCL(degoct[0], fract(note), this->mtof_01_innerScala_refFreq);
    }

    this->mtof_01_innerScala_updateLast(note, out);
    return out;
}

number mtof_01_innerScala_ftom(number hz) {
    if (hz <= 0.0) {
        return 0.0;
    }

    if ((bool)(this->mtof_01_innerScala_lastValid) && this->mtof_01_innerScala_lastFreq == hz) {
        return this->mtof_01_innerScala_lastNote;
    }

    array<number, 2> df = this->mtof_01_innerScala_hztodeg(hz);
    Int degree = (Int)(df[0]);
    number frac = df[1];
    number out = 0;

    if (this->mtof_01_innerScala_kbmSize == 0) {
        out = this->mtof_01_innerScala_kbmMid + degree;
    } else {
        array<Int, 2> octdeg = this->mtof_01_innerScala_octdegree(degree, this->mtof_01_innerScala_kbmOctaveDegree);
        number oct = (number)(octdeg[0]);
        Int index = (Int)(octdeg[1]);
        Index entry = 0;

        for (Index i = 0; i < this->mtof_01_innerScala_kbmMapSize; i++) {
            if (index == this->mtof_01_innerScala_kbmValid[(Index)(i + this->mtof_01_innerScala_KBM_MAP_OFFSET)]) {
                entry = i;
                break;
            }
        }

        out = oct * this->mtof_01_innerScala_kbmSize + entry + this->mtof_01_innerScala_kbmMid;
    }

    out = out + frac;
    this->mtof_01_innerScala_updateLast(out, hz);
    return this->mtof_01_innerScala_lastNote;
}

template<typename LISTTYPE = list> Int mtof_01_innerScala_updateScale(const LISTTYPE& scl) {
    if (scl->length < 1) {
        return 0;
    }

    number sclDataEntries = scl[0] * 2 + 1;

    if (sclDataEntries <= scl->length) {
        this->mtof_01_innerScala_lastValid = false;
        this->mtof_01_innerScala_sclExpMul = {};
        number last = 1;

        for (Index i = 1; i < sclDataEntries; i += 2) {
            const number c = (const number)(scl[(Index)(i + 0)]);
            const number d = (const number)(scl[(Index)(i + 1)]);

            if (d <= 0) {
                last = c / (number)1200;
            } else {
                last = rnbo_log2(c / d);
            }

            this->mtof_01_innerScala_sclExpMul->push(last);
        }

        this->mtof_01_innerScala_sclOctaveMul = last;
        this->mtof_01_innerScala_sclEntryCount = (Int)(this->mtof_01_innerScala_sclExpMul->length);

        if (scl->length >= sclDataEntries + 3) {
            this->mtof_01_innerScala_kbmMid = (Int)(scl[(Index)(sclDataEntries + 2)]);
            this->mtof_01_innerScala_kbmRefNum = (Int)(scl[(Index)(sclDataEntries + 1)]);
            this->mtof_01_innerScala_kbmRefFreq = scl[(Index)(sclDataEntries + 0)];
            this->mtof_01_innerScala_kbmSize = (Int)(0);
        }

        this->mtof_01_innerScala_updateRefFreq();
        return 1;
    }

    return 0;
}

template<typename LISTTYPE = list> Int mtof_01_innerScala_updateMap(const LISTTYPE& kbm) {
    list _kbm = kbm;

    if (_kbm->length == 1 && _kbm[0] == 0.0) {
        _kbm = {0.0, 0.0, 0.0, 60.0, 69.0, 440.0};
    }

    if (_kbm->length >= 6 && _kbm[0] >= 0.0) {
        this->mtof_01_innerScala_lastValid = false;
        Index size = (Index)(_kbm[0]);
        Int octave = 12;

        if (_kbm->length > 6) {
            octave = (Int)(_kbm[6]);
        }

        if (size > 0 && _kbm->length < this->mtof_01_innerScala_KBM_MAP_OFFSET) {
            return 0;
        }

        this->mtof_01_innerScala_kbmSize = (Int)(size);
        this->mtof_01_innerScala_kbmMin = (Int)(_kbm[1]);
        this->mtof_01_innerScala_kbmMax = (Int)(_kbm[2]);
        this->mtof_01_innerScala_kbmMid = (Int)(_kbm[3]);
        this->mtof_01_innerScala_kbmRefNum = (Int)(_kbm[4]);
        this->mtof_01_innerScala_kbmRefFreq = _kbm[5];
        this->mtof_01_innerScala_kbmOctaveDegree = octave;
        this->mtof_01_innerScala_kbmValid = _kbm;
        this->mtof_01_innerScala_kbmMapSize = (_kbm->length - this->mtof_01_innerScala_KBM_MAP_OFFSET > _kbm->length ? _kbm->length : (_kbm->length - this->mtof_01_innerScala_KBM_MAP_OFFSET < 0 ? 0 : _kbm->length - this->mtof_01_innerScala_KBM_MAP_OFFSET));
        this->mtof_01_innerScala_updateRefFreq();
        return 1;
    }

    return 0;
}

void mtof_01_innerScala_updateLast(number note, number freq) {
    this->mtof_01_innerScala_lastValid = true;
    this->mtof_01_innerScala_lastNote = note;
    this->mtof_01_innerScala_lastFreq = freq;
}

array<number, 2> mtof_01_innerScala_hztodeg(number hz) {
    number hza = rnbo_abs(hz);

    number octave = rnbo_floor(
        rnbo_log2(hza / this->mtof_01_innerScala_refFreq) / this->mtof_01_innerScala_sclOctaveMul
    );

    Int i = 0;
    number frac = 0;
    number n = 0;

    for (; i < this->mtof_01_innerScala_sclEntryCount; i++) {
        number c = this->mtof_01_innerScala_applySCLOctIndex(octave, i + 0, 0.0, this->mtof_01_innerScala_refFreq);
        n = this->mtof_01_innerScala_applySCLOctIndex(octave, i + 1, 0.0, this->mtof_01_innerScala_refFreq);

        if (c <= hza && hza < n) {
            if (c != hza) {
                frac = rnbo_log2(hza / c) / rnbo_log2(n / c);
            }

            break;
        }
    }

    if (i == this->mtof_01_innerScala_sclEntryCount && n != hza) {
        number c = n;
        n = this->mtof_01_innerScala_applySCLOctIndex(octave + 1, 0, 0.0, this->mtof_01_innerScala_refFreq);
        frac = rnbo_log2(hza / c) / rnbo_log2(n / c);
    }

    number deg = i + octave * this->mtof_01_innerScala_sclEntryCount;

    {
        deg = rnbo_fround((deg + frac) * 1 / (number)1) * 1;
        frac = 0.0;
    }

    return {deg, frac};
}

array<Int, 2> mtof_01_innerScala_octdegree(Int degree, Int count) {
    Int octave = 0;
    Int index = 0;

    if (degree < 0) {
        octave = -(1 + (-1 - degree) / count);
        index = -degree % count;

        if (index > 0) {
            index = count - index;
        }
    } else {
        octave = degree / count;
        index = degree % count;
    }

    return {octave, index};
}

array<Int, 2> mtof_01_innerScala_applyKBM(number note) {
    if ((this->mtof_01_innerScala_kbmMin == this->mtof_01_innerScala_kbmMax && this->mtof_01_innerScala_kbmMax == 0) || (note >= this->mtof_01_innerScala_kbmMin && note <= this->mtof_01_innerScala_kbmMax)) {
        Int degree = (Int)(rnbo_floor(note - this->mtof_01_innerScala_kbmMid));

        if (this->mtof_01_innerScala_kbmSize == 0) {
            return {degree, 1};
        }

        array<Int, 2> octdeg = this->mtof_01_innerScala_octdegree(degree, this->mtof_01_innerScala_kbmSize);
        Int octave = (Int)(octdeg[0]);
        Index index = (Index)(octdeg[1]);

        if (this->mtof_01_innerScala_kbmMapSize > index) {
            degree = (Int)(this->mtof_01_innerScala_kbmValid[(Index)(this->mtof_01_innerScala_KBM_MAP_OFFSET + index)]);

            if (degree >= 0) {
                return {degree + octave * this->mtof_01_innerScala_kbmOctaveDegree, 1};
            }
        }
    }

    return {-1, 0};
}

number mtof_01_innerScala_applySCL(Int degree, number frac, number refFreq) {
    array<Int, 2> octdeg = this->mtof_01_innerScala_octdegree(degree, this->mtof_01_innerScala_sclEntryCount);
    return this->mtof_01_innerScala_applySCLOctIndex(octdeg[0], octdeg[1], frac, refFreq);
}

number mtof_01_innerScala_applySCLOctIndex(number octave, Int index, number frac, number refFreq) {
    number p = 0;

    if (index > 0) {
        p = this->mtof_01_innerScala_sclExpMul[(Index)(index - 1)];
    }

    if (frac > 0) {
        p = this->linearinterp(frac, p, this->mtof_01_innerScala_sclExpMul[(Index)index]);
    } else if (frac < 0) {
        p = this->linearinterp(-frac, this->mtof_01_innerScala_sclExpMul[(Index)index], p);
    }

    return refFreq * rnbo_pow(2, p + octave * this->mtof_01_innerScala_sclOctaveMul);
}

void mtof_01_innerScala_updateRefFreq() {
    this->mtof_01_innerScala_lastValid = false;
    Int refOffset = (Int)(this->mtof_01_innerScala_kbmRefNum - this->mtof_01_innerScala_kbmMid);

    if (refOffset == 0) {
        this->mtof_01_innerScala_refFreq = this->mtof_01_innerScala_kbmRefFreq;
    } else {
        Int base = (Int)(this->mtof_01_innerScala_kbmSize);

        if (base < 1) {
            base = this->mtof_01_innerScala_sclEntryCount;
        }

        array<Int, 2> octdeg = this->mtof_01_innerScala_octdegree(refOffset, base);
        number oct = (number)(octdeg[0]);
        Int index = (Int)(octdeg[1]);

        if (base > 0) {
            oct = oct + rnbo_floor(index / base);
            index = index % base;
        }

        if (index >= 0 && index < this->mtof_01_innerScala_kbmSize) {
            if (index < (Int)(this->mtof_01_innerScala_kbmMapSize)) {
                index = (Int)(this->mtof_01_innerScala_kbmValid[(Index)((Index)(index) + this->mtof_01_innerScala_KBM_MAP_OFFSET)]);
            } else {
                index = -1;
            }
        }

        if (index < 0 || index > (Int)(this->mtof_01_innerScala_sclExpMul->length))
            {} else {
            number p = 0;

            if (index > 0) {
                p = this->mtof_01_innerScala_sclExpMul[(Index)(index - 1)];
            }

            this->mtof_01_innerScala_refFreq = this->mtof_01_innerScala_kbmRefFreq / rnbo_pow(2, p + oct * this->mtof_01_innerScala_sclOctaveMul);
        }
    }
}

void mtof_01_innerScala_reset() {
    this->mtof_01_innerScala_lastValid = false;
    this->mtof_01_innerScala_lastNote = 0;
    this->mtof_01_innerScala_lastFreq = 0;
    this->mtof_01_innerScala_sclEntryCount = 0;
    this->mtof_01_innerScala_sclOctaveMul = 1;
    this->mtof_01_innerScala_sclExpMul = {};
    this->mtof_01_innerScala_kbmValid = {0, 0, 0, 60, 69, 440};
    this->mtof_01_innerScala_kbmMid = 60;
    this->mtof_01_innerScala_kbmRefNum = 69;
    this->mtof_01_innerScala_kbmRefFreq = 440;
    this->mtof_01_innerScala_kbmSize = 0;
    this->mtof_01_innerScala_kbmMin = 0;
    this->mtof_01_innerScala_kbmMax = 0;
    this->mtof_01_innerScala_kbmOctaveDegree = 12;
    this->mtof_01_innerScala_kbmMapSize = 0;
    this->mtof_01_innerScala_refFreq = 261.63;
}

void mtof_01_init() {
    this->mtof_01_innerScala_update(this->mtof_01_scale, this->mtof_01_map);
}

number cycle_tilde_01_ph_next(number freq, number reset) {
    {
        {
            if (reset >= 0.)
                this->cycle_tilde_01_ph_currentPhase = reset;
        }
    }

    number pincr = freq * this->cycle_tilde_01_ph_conv;

    if (this->cycle_tilde_01_ph_currentPhase < 0.)
        this->cycle_tilde_01_ph_currentPhase = 1. + this->cycle_tilde_01_ph_currentPhase;

    if (this->cycle_tilde_01_ph_currentPhase > 1.)
        this->cycle_tilde_01_ph_currentPhase = this->cycle_tilde_01_ph_currentPhase - 1.;

    number tmp = this->cycle_tilde_01_ph_currentPhase;
    this->cycle_tilde_01_ph_currentPhase += pincr;
    return tmp;
}

void cycle_tilde_01_ph_reset() {
    this->cycle_tilde_01_ph_currentPhase = 0;
}

void cycle_tilde_01_ph_dspsetup() {
    this->cycle_tilde_01_ph_conv = (number)1 / this->sr;
}

void cycle_tilde_01_dspsetup(bool force) {
    if ((bool)(this->cycle_tilde_01_setupDone) && (bool)(!(bool)(force)))
        return;

    this->cycle_tilde_01_phasei = 0;
    this->cycle_tilde_01_f2i = (number)4294967296 / this->sr;
    this->cycle_tilde_01_wrap = (Int)(this->cycle_tilde_01_buffer->getSize()) - 1;
    this->cycle_tilde_01_setupDone = true;
    this->cycle_tilde_01_ph_dspsetup();
}

void cycle_tilde_01_bufferUpdated() {
    this->cycle_tilde_01_wrap = (Int)(this->cycle_tilde_01_buffer->getSize()) - 1;
}

void message_01_init() {
    this->message_01_set_set(listbase<number, 6>{1, 100, 1, 500, 0, 200});
}

void globaltransport_advance() {}

void globaltransport_dspsetup(bool ) {}

bool stackprotect_check() {
    this->stackprotect_count++;

    if (this->stackprotect_count > 128) {
        console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
        return true;
    }

    return false;
}

Index getPatcherSerial() const {
    return 0;
}

void sendParameter(ParameterIndex index, bool ignoreValue) {
    this->getEngine()->notifyParameterValueChanged(index, (ignoreValue ? 0 : this->getParameterValue(index)), ignoreValue);
}

void scheduleParamInit(ParameterIndex index, Index order) {
    this->paramInitIndices->push(index);
    this->paramInitOrder->push(order);
}

void processParamInitEvents() {
    this->listquicksort(
        this->paramInitOrder,
        this->paramInitIndices,
        0,
        (int)(this->paramInitOrder->length - 1),
        true
    );

    for (Index i = 0; i < this->paramInitOrder->length; i++) {
        this->getEngine()->scheduleParameterBang(this->paramInitIndices[i], 0);
    }
}

void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
    RNBO_UNUSED(inProcess);
    RNBO_UNUSED(engine);
    this->_currentTime = time;
    auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));

    if (offset >= (SampleIndex)(this->vs))
        offset = (SampleIndex)(this->vs) - 1;

    if (offset < 0)
        offset = 0;

    this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
}

void assign_defaults()
{
    notein_01_channel = 0;
    mtof_01_base = 440;
    cycle_tilde_01_frequency = 0;
    cycle_tilde_01_phase_offset = 0;
    dspexpr_01_in1 = 0;
    dspexpr_01_in2 = 0;
    select_01_test1 = 0;
    linetilde_01_time = 0;
    linetilde_01_keepramp = false;
    _currentTime = 0;
    audioProcessSampleCount = 0;
    sampleOffsetIntoNextAudioBuffer = 0;
    zeroBuffer = nullptr;
    dummyBuffer = nullptr;
    signals[0] = nullptr;
    signals[1] = nullptr;
    didAllocateSignals = 0;
    vs = 0;
    maxvs = 0;
    sr = 44100;
    invsr = 0.000022675736961451248;
    notein_01_status = 0;
    notein_01_byte1 = -1;
    notein_01_inchan = 0;
    mtof_01_innerMtoF_lastInValue = 0;
    mtof_01_innerMtoF_lastOutValue = 0;
    mtof_01_innerMtoF_lastTuning = 0;
    mtof_01_innerScala_lastValid = false;
    mtof_01_innerScala_lastNote = 0;
    mtof_01_innerScala_lastFreq = 0;
    mtof_01_innerScala_sclEntryCount = 0;
    mtof_01_innerScala_sclOctaveMul = 1;
    mtof_01_innerScala_kbmValid = { 0, 0, 0, 60, 69, 440 };
    mtof_01_innerScala_kbmMid = 60;
    mtof_01_innerScala_kbmRefNum = 69;
    mtof_01_innerScala_kbmRefFreq = 440;
    mtof_01_innerScala_kbmSize = 0;
    mtof_01_innerScala_kbmMin = 0;
    mtof_01_innerScala_kbmMax = 0;
    mtof_01_innerScala_kbmOctaveDegree = 12;
    mtof_01_innerScala_kbmMapSize = 0;
    mtof_01_innerScala_refFreq = 261.63;
    cycle_tilde_01_wrap = 0;
    cycle_tilde_01_ph_currentPhase = 0;
    cycle_tilde_01_ph_conv = 0;
    cycle_tilde_01_setupDone = false;
    linetilde_01_currentValue = 0;
    globaltransport_tempo = nullptr;
    globaltransport_state = nullptr;
    stackprotect_count = 0;
    _voiceIndex = 0;
    _noteNumber = 0;
    isMuted = 1;
}

    // data ref strings
    struct DataRefStrings {
    	static constexpr auto& name0 = "RNBODefaultMtofLookupTable256";
    	static constexpr auto& file0 = "";
    	static constexpr auto& tag0 = "buffer~";
    	static constexpr auto& name1 = "RNBODefaultSinus";
    	static constexpr auto& file1 = "";
    	static constexpr auto& tag1 = "buffer~";
    	DataRefStrings* operator->() { return this; }
    	const DataRefStrings* operator->() const { return this; }
    };

    DataRefStrings dataRefStrings;

// member variables

    number notein_01_channel;
    list mtof_01_midivalue;
    list mtof_01_scale;
    list mtof_01_map;
    number mtof_01_base;
    number cycle_tilde_01_frequency;
    number cycle_tilde_01_phase_offset;
    number dspexpr_01_in1;
    number dspexpr_01_in2;
    number select_01_test1;
    list message_01_set;
    list linetilde_01_segments;
    number linetilde_01_time;
    number linetilde_01_keepramp;
    MillisecondTime _currentTime;
    ENGINE _internalEngine;
    UInt64 audioProcessSampleCount;
    Index sampleOffsetIntoNextAudioBuffer;
    signal zeroBuffer;
    signal dummyBuffer;
    SampleValue * signals[2];
    bool didAllocateSignals;
    Index vs;
    Index maxvs;
    number sr;
    number invsr;
    Int notein_01_status;
    Int notein_01_byte1;
    Int notein_01_inchan;
    number mtof_01_innerMtoF_lastInValue;
    number mtof_01_innerMtoF_lastOutValue;
    number mtof_01_innerMtoF_lastTuning;
    SampleBufferRef mtof_01_innerMtoF_buffer;
    const Index mtof_01_innerScala_KBM_MAP_OFFSET = 7;
    bool mtof_01_innerScala_lastValid;
    number mtof_01_innerScala_lastNote;
    number mtof_01_innerScala_lastFreq;
    Int mtof_01_innerScala_sclEntryCount;
    number mtof_01_innerScala_sclOctaveMul;
    list mtof_01_innerScala_sclExpMul;
    list mtof_01_innerScala_kbmValid;
    Int mtof_01_innerScala_kbmMid;
    Int mtof_01_innerScala_kbmRefNum;
    number mtof_01_innerScala_kbmRefFreq;
    Int mtof_01_innerScala_kbmSize;
    Int mtof_01_innerScala_kbmMin;
    Int mtof_01_innerScala_kbmMax;
    Int mtof_01_innerScala_kbmOctaveDegree;
    Index mtof_01_innerScala_kbmMapSize;
    number mtof_01_innerScala_refFreq;
    SampleBufferRef cycle_tilde_01_buffer;
    Int cycle_tilde_01_wrap;
    UInt32 cycle_tilde_01_phasei;
    SampleValue cycle_tilde_01_f2i;
    number cycle_tilde_01_ph_currentPhase;
    number cycle_tilde_01_ph_conv;
    bool cycle_tilde_01_setupDone;
    list linetilde_01_activeRamps;
    number linetilde_01_currentValue;
    signal globaltransport_tempo;
    signal globaltransport_state;
    number stackprotect_count;
    DataRef RNBODefaultMtofLookupTable256;
    DataRef RNBODefaultSinus;
    Index _voiceIndex;
    Int _noteNumber;
    Index isMuted;
    indexlist paramInitIndices;
    indexlist paramInitOrder;
    bool _isInitialized = false;
};

static PatcherInterface* creaternbomatic()
{
    return new rnbomatic<EXTERNALENGINE>();
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" PatcherFactoryFunctionPtr GetPatcherFactoryFunction()
#else
extern "C" PatcherFactoryFunctionPtr rnbomaticFactoryFunction()
#endif
{
    return creaternbomatic;
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" void SetLogger(Logger* logger)
#else
void rnbomaticSetLogger(Logger* logger)
#endif
{
    console = logger;
}

} // end RNBO namespace

