#ifndef DRR_QUEUE_DISC
#define DRR_QUEUE_DISC

#include "ns3/queue-disc.h"
#include "ns3/object-factory.h"
#include <list>
#include <map>

namespace ns3 {

    /**
    * \ingroup traffic-control
    *
    * \brief A flow queue used by the FqCoDel queue disc
    */

    class DrrFlow : public QueueDiscClass {
        public:
        /**
        * \brief Get the type ID.
        * \return the object TypeId
        */
        static TypeId GetTypeId (void);
        /**
        * \brief DrrFlow constructor
        */
        DrrFlow ();

        virtual ~DrrFlow ();

        /**
        * \enum FlowStatus
        * \brief Used to determine the status of this flow queue. Either active or inactive
        */
        enum FlowStatus
        {
            ACTIVE,
            INACTIVE
        };

        /**
        * \brief Set the deficit for this flow
        * \param deficit the deficit for this flow
        */
        void SetDeficit (uint32_t deficit);
        /**
        * \brief Get the deficit for this flow
        * \return the deficit for this flow
        */
        int32_t GetDeficit (void) const;
        /**
        * \brief Increase the deficit for this flow
        * \param deficit the amount by which the deficit is to be increased
        */
        void IncreaseDeficit (int32_t deficit);
        /**
        * \brief Set the status for this flow
        * \param status the status for this flow
        */
        void SetStatus (FlowStatus status);
        /**
        * \brief Get the status of this flow
        * \return the status of this flow
        */
        FlowStatus GetStatus (void) const;
        /**
        * \brief Set the index for this flow
        * \param index the index for this flow
        */
        void SetIndex (uint32_t index);
        /**
        * \brief Get the index of this flow
        * \return the index of this flow
        */
        uint32_t GetIndex (void) const;

    private:
        int32_t m_deficit;    //!< the deficit for this flow
    FlowStatus m_status;  //!< the status of this flow
    uint32_t m_index;     //!< the index for this flow
    };


    /**
    * \ingroup traffic-control
    *
    * \brief A Drr packet queue disc
    */

    class DrrQueueDisc : public QueueDisc {
        public:
        /**
        * \brief Get the type ID.
        * \return the object TypeId
        */
        static TypeId GetTypeId (void);
        /**
        * \brief DrrQueueDisc constructor
        */
        DrrQueueDisc ();

        virtual ~DrrQueueDisc ();

        /**
        * \brief Set the quantum value.
        *
        * \param quantum The number of bytes each queue gets to dequeue on each round of the scheduling algorithm
        */
        void SetQuantum (uint32_t quantum);

        /**
        * \brief Get the quantum value.
        *
        * \returns The number of bytes each queue gets to dequeue on each round of the scheduling algorithm
        */
        uint32_t GetQuantum (void) const;

        // Reasons for dropping packets
        static constexpr const char* UNCLASSIFIED_DROP = "Unclassified drop";  //!< No packet filter able to classify packet
        static constexpr const char* OVERLIMIT_DROP = "Overlimit drop";        //!< Overlimit dropped packets

    private:
        virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
        virtual Ptr<QueueDiscItem> DoDequeue (void);
        virtual bool CheckConfig (void);
        virtual void InitializeParams (void);

        /**
        * \brief Drop a packet from the head of the queue with the largest current byte count
        * \return the index of the queue with the largest current byte count
        */
        uint32_t DrrDrop (void);

        uint32_t m_quantum;        //!< Deficit assigned to flows at each round
        uint32_t m_flows;          //!< Number of flow queues
        uint32_t m_dropBatchSize;  //!< Max number of packets dropped from the fat flow


        std::list<Ptr<DrrFlow> > m_activeList;    //!< List of active flows

        std::map<uint32_t, uint32_t> m_flowsIndices;    //!< Map with the index of class for each flow

        ObjectFactory m_flowFactory;         //!< Factory to create a new flow
        ObjectFactory m_queueDiscFactory;    //!< Factory to create a new queue
    };

} // namespace ns3

#endif /* DRR_QUEUE_DISC */