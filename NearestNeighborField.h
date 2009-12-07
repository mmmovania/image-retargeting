#pragma once

#include "Image.h"
#include "Convert.h"
#include "Random.h"
#include "TypeTraits.h"
#include "Point2D.h"
#include "Rectangle.h"
#include "Parallel.h"
#include "Queue.h"
#include "Alpha.h"

namespace IRL
{
    // Possible types of initial NNF field
    enum InitialNNFType
    {
        RandomField, 
        SmoothField,
        PredefinedField
    };

    typedef Image<Point16> OffsetField;

    // NNF stands for NearestNeighborField
    template<class PixelType, bool UseSourceMask>
    class NNF
    {
        typedef typename PixelType::DistanceType DistanceType;

    public:
        Image<PixelType> Source;      // B
        Image<Alpha8>    SourceMask;  // which pixel from source is allowed to use
        Image<PixelType> Target;      // A
        OffsetField      Field;       // Result of the algorithm's work
        InitialNNFType InitialOffsetField;

    public:
        NNF();

        // Make one iteration of the algorithm.
        void Iteration(bool parallel = true);
        // Save resulting NNF into file.
        void Save(const std::string& path);

    private:
        // Initializes the algorithm before first iteration.
        void Initialize();
        // Initializes initial NNF with random values
        void RandomFill();
        // Initialized initial NNF with values that arise when interpolate from target to source
        void SmoothFill();

        // If pixel gets in masked zone, tries to move it out of it
        inline void CheckThatNotMasked(int32_t& sx, int32_t& sy);

        // Fills _superPatches vector
        void BuildSuperPatches();
        // Fills _cache variable with initial value
        void PrepareCache(int left, int top, int right, int bottom);

        // Sequential complete iteration over target image's region
        void Iteration(int left, int top, int right, int bottom, int iteration);

        // Perform direct scan order step over target image's region
        void DirectScanOrder(int left, int top, int right, int bottom);
        // Perform reverse scan order step over target image's region
        void ReverseScanOrder(int left, int top, int right, int bottom);

        // Propagation.
        // Direction +1 for direct scan order, -1 for reverse one.
        // LeftAvailable == true if can propagate horizontally.
        // UpAvailable == true if can propagate vertically
        template<int Direction, bool LeftAvailable, bool UpAvailable>
        void Propagate(const Point32& target);

        // Random search step on pixel
        inline void RandomSearch(const Point32& target);

        #pragma region Propagate support methods
        force_inline DistanceType MoveDistanceByDx(const Point32& target, int dx);
        force_inline DistanceType MoveDistanceByDy(const Point32& target, int dy);

        template<int Direction> bool CheckX(int x); // no implementation here
        template<> inline bool CheckX<-1>(int x) { return x > _targetRect.Left; }
        template<> inline bool CheckX<+1>(int x) { return x < _targetRect.Right - 1; }

        template<int Direction> bool CheckY(int x);
        template<> inline bool CheckY<-1>(int y) { return y > _targetRect.Top; }
        template<> inline bool CheckY<+1>(int y) { return y < _targetRect.Bottom - 1; }
        #pragma endregion

        // Calculate distance from target to source patch.
        // If EarlyTermination == true use 'known' to stop calculation once distance > known
        template<bool EarlyTermination>
        force_inline DistanceType Distance(const Point32& targetPatch, const Point32& sourcePatch, DistanceType known = 0);

        // Return distance between pixels
        force_inline DistanceType PixelDistance(int sx, int sy, int tx, int ty);

        // handy shortcut
        force_inline Point16& f(const Point32& p) { return Field(p.x, p.y); }

    private:
        // Used to implement multithreading
        // Unit of the thread processing
        struct SuperPatch
        {
            int Left;
            int Top;
            int Right;
            int Bottom;

            SuperPatch* LeftNeighbor;
            SuperPatch* TopNeighbor;
            SuperPatch* RightNeighbor;
            SuperPatch* BottomNeighbor;

            bool AddedToQueue;
            bool Processed;
        };

        // Used to implement multithreading.
        // All tasks share queue with superpatches.
        class IterationTask :
            public Parallel::Runnable
        {
        public:
            IterationTask();
            void Initialize(NNF* owner, Queue<SuperPatch>* queue, int iteration, Mutex* lock);
            virtual void Run();
        private:
            inline void VisitRightPatch(SuperPatch* patch);
            inline void VisitBottomPatch(SuperPatch* patch);
            inline void VisitLeftPatch(SuperPatch* patch);
            inline void VisitTopPatch(SuperPatch* patch);
        private:
            NNF* _owner;
            Queue<SuperPatch>* _queue;
            int _iteration;
            Mutex* _lock;
        };

    private:
        // Holds current best distances
        Image<DistanceType> _cache;
        // Random generator. Note: in parallel mode result is also depends on thread scheduling
        Random _random;
        // Current iteration number (starts with 0)
        int _iteration;

        // Rectangle with allowed source patch centers
        Rectangle<int32_t> _sourceRect;
        // Rectangle with allowed target patch centers
        Rectangle<int32_t> _targetRect;

        // Multithreading support
        Queue<SuperPatch> _superPatchQueue;
        std::vector<SuperPatch> _superPatches;
        SuperPatch* _topLeftSuperPatch;
        SuperPatch* _bottomRightSuperPatch;
        Mutex _lock;
    };
}

#include "NearestNeighborField.inl"