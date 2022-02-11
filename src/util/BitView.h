#pragma once

// This is a utility class for reading entries encoded in bits, whic
// are aligned to uint64 boundaries and written as big-endian.
// This helps us read values by firest transforming the data back to
// little endian and making it easy to read variable-sized bit values.
// The format of the bits is such that the entries are shifted all the way to
// the left of a field. For example, an initial 32-bit value will be found in the MSbits of the first field.
class BitReader
{
public:
    // bytesBE must be rounded-up to 64-bit boundaries
    // This expects the bytes to be encoded as 64-bit big-endian fields.
    // The last bytes will be shifted to the right then swaped as 64-bits as well.
    //-----------------------------------------------------------
    inline BitReader( uint64* bytesBE, size_t sizeBits )
        : _fields  ( bytesBE  )
        , _sizeBits( sizeBits )
        , _position( 0 )
    {
        ASSERT( sizeBits / 64 * 64 == sizeBits );
        ASSERT( sizeBits <= (size_t)std::numeric_limits<ssize_t>::max() );

        const size_t fieldCount = sizeBits / 64;

        for( uint64 i = 0; i < fieldCount; i++ )
            bytesBE[i] = Swap64( bytesBE[i] );
        
        // Also swap any remainder bytes
        const size_t bitsRemainder = sizeBits - fieldCount * 64;
        if( bitsRemainder )
            bytesBE[fieldCount] = Swap64( bytesBE[fieldCount] << ( 64 - bitsRemainder ) );
    }

    // Read 64 bits or less
    //-----------------------------------------------------------
    uint64 ReadBits64( const uint32 bitCount )
    {
        ASSERT( bitCount <= 64 );
        ASSERT( _position + bitCount <= _sizeBits );

        const uint64 fieldIndex    = _position >> 6; // _position / 64
        const uint32 bitsAvailable = ( ( fieldIndex + 1 ) * 64 ) - _position;
        const uint32 shift         = std::max( bitCount, bitsAvailable ) - bitCount;

        uint64 value = _fields[fieldIndex] >> shift;

        if( bitsAvailable < bitCount )
        {
            // Have to read one more field
            const uint32 bitsNeeded = bitCount - bitsAvailable;
            value = ( value << bitsNeeded ) | ( _fields[fieldIndex+1] >> ( 64 - bitsNeeded ) );
        }

        // Mask-out part of the fields we don't need
        value &= ( 0xFFFFFFFFFFFFFFFFull >> (64 - bitCount ) );

        _position += bitCount;
        return value;
    }

    // Read 128 bits or less
    //-----------------------------------------------------------
    uint128 ReadBits128( const uint32 bitCount )
    {
        ASSERT( bitCount <= 128 );
        ASSERT( _position + bitCount <= _sizeBits );

        const uint64 fieldIndex    = _position >> 6; // _position / 64
        const uint32 bitsAvailable = ( ( fieldIndex + 1 ) * 64 ) - _position;
        const uint32 shift         = std::max( bitCount, bitsAvailable ) - bitCount;

        uint128 value = _fields[fieldIndex] >> shift;

        if( bitsAvailable < bitCount )
        {
            // Have to read one more field
            const uint32 bitsNeeded = bitCount - bitsAvailable;

            if( bitsNeeded > 64 )
            {
                // Need data from 2 more fields
                const uint32 lastFieldBitsNeeded = bitsNeeded - 64;
                value = ( value << bitsNeeded ) | ( _fields[fieldIndex+1] << lastFieldBitsNeeded );
                value |= _fields[fieldIndex+2] >> ( 64 - lastFieldBitsNeeded );
            }
            else
            {
                // Only need data from 1 more field
                value = ( value << bitsNeeded ) | ( _fields[fieldIndex+1] >> ( 64 - bitsNeeded ) );
            }
        }

        // Mask-out part of the fields we don't need
        value &= ( uint128( 0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull ) >> ( 128 - bitCount ) );

        _position += bitCount;
        return value;
    }
    

    //-----------------------------------------------------------
    // Read bytes, but convert our fields to BigEndian before
    // storing them into outBytes. This is for compatibility
    // with the way chiapos stores data in their plots
    //-----------------------------------------------------------
    // void ReadBytes( size_t byteCount, byte* outBytes )
    // {
    //     ASSERT( byteCount <= (size_t)std::numeric_limits<ssize_t>::max() );
    //     ASSERT( byteCount * 8 < _sizeBits );
    //     const size_t fieldCount = CDiv( byteCount, 8 );
        
    //     const size_t fieldIndex = _position >> 6; // _position / 64
    //     ASSERT( fieldIndex < _position );

    //     const uint32 bitIndex = (uint32)( _position - fieldIndex * 64 ); // This is the local bit position inside the current field.

    //     // Fast path for when the bit index is 0
    //     if( bitIndex == 0 )
    //     {
    //         const uint64* fieldReader = _fields + fieldIndex;
    //         uint64*       fieldWriter = (uint64*)outBytes;

    //         const uint64* end = fieldReader + fieldCount;
    //         do {
    //             *fieldWriter++ = Swap64( *fieldReader++ );
    //         } 
    //         while( fieldReader < end );

    //         // Read remaining bytes that are not field-aligned
    //         const size_t bytesRemaining = byteCount - fieldCount * 8;
    //         if( bytesRemaining )
    //         {
    //             const uint64 remainder = Swap64( *fieldReader );
    //             memcpy( fieldWriter, &remainder, sizeof( remainder ) );
    //         }
    //     }
    //     else
    //     {
    //         // #TODO: Implement this when we need it
    //         ASSERT( 0 );
    //         // for( size_t i = 0; i < fieldCount; i++ )
    //         // {}
    //     }
    // }


private:
    uint64* _fields  ;  // Our fields buffer
    uint64  _position;  // Read poisition in bits
    size_t  _sizeBits;  // Size of the how much data we currently have in bits
};

// class BitView
// {
// public:
//     //-----------------------------------------------------------
//     BitView( uint64* buffer, size_t size, size_t entrySizeBits )
//         : _fields   ( buffer )
//         , _size     ( size   )
//         , _EntrySize( entrySizeBits )
//     {
//         ASSERT( buffer );
//         ASSERT( size   );
//         ASSERT( entrySizeBits );

//         // #TODO: Allow entries greater than 64 bits...
//         static_assert( entrySizeBits > 0 && entrySizeBits < 65, "_EntrySize must be > 0 < 65" );
//     }

//     // Read an entry of _EntrySize bits at index.
//     // Unsafe, this will not check bounds on release mode.
//     //-----------------------------------------------------------
//     uint64 Get( uint64 index )
//     {

//     }

//     // Write an entry of _EntrySize bits at index.
//     // Unsafe, this will not check bounds on release mode.
//     //-----------------------------------------------------------
//     inline void Set( uint64 index, uint64 value )
//     {

//     }

//     uint64 operator[] ( uint64 index );

// private:
//     uint64 GetWordIndex( uint64 entryIndex );

// private:
//     uint64* _fields;
//     size_t  _EntrySize; // Size of a single entry, in bits
//     size_t  _size ;     // Maximum size as uint64 entries. 
//                         // That is, how many uint64 entries can we hold.
// };


/// <summary>
/// Static-typed version of BitView
/// _EntrySize must be in bits.
/// </summary>
template<size_t _EntrySize>
class BitViewT
{
public:
    BitViewT( uint64* buffer, size_t64 size );

    // Read an entry of _EntrySize bits at index.
    // Unsafe, this will not check bounds on release mode.
    uint64 Get( uint64 index );

    // Write an entry of _EntrySize bits at index.
    // Unsafe, this will not check bounds on release mode.
    void   Set( uint64 index, uint64 value );

    uint64 operator[] ( uint64 index );

private:
    uint64 GetWordIndex( uint64 entryIndex );

private:
    uint64*     _fields;
    size_t64    _size ;     // Maximum size as uint64 entries. 
                            // That is, how many uint64 entries can we hold.
};

// Bits occupied by the entry size
#define _EntryBits      ( ( 1ull << _EntrySize ) -1 )

// Bits not occupied by the entry size
#define _InvEntryBits   ( ~_EntryBits )
#define _BitInv( x ) (~(x))

//-----------------------------------------------------------
template<size_t _EntrySize>
inline BitViewT<_EntrySize>::BitViewT( uint64* buffer, size_t64 size )
    : _fields( buffer )
    , _size  ( size   )
{
    ASSERT( buffer );
    ASSERT( size   );

    static_assert( _EntrySize > 0 && _EntrySize < 65, "_EntrySize must be > 0 < 65" );
}


//-----------------------------------------------------------
template<size_t _EntrySize>
inline uint64 BitViewT<_EntrySize>::Get( uint64 index )
{
    ASSERT( index * _EntrySize + _EntrySize <= _size * 64 );

    const uint64 wordIdx = GetWordIndex( index );

    // 64 because our 'word' size is 64-bits/8 bytes
    const uint64 bitStart = ( index * _EntrySize ) - ( wordIdx * 64 );
    const uint64 bitCount = 64 - bitStart;    // Bits available in the WORD
    
    uint64 value = _fields[wordIdx] >> bitStart;

    // Need to read next field?
    if( bitCount < _EntrySize )
        value |= _fields[wordIdx+1] << bitCount;
    
    return value & _EntryBits;
}

//-----------------------------------------------------------
template<size_t _EntrySize>
inline void BitViewT<_EntrySize>::Set( uint64 index, uint64 value )
{
    ASSERT( index * _EntrySize + _EntrySize <= _size * 64 );

    const uint64 wordIdx = GetWordIndex( index );
    
    // 64 because our 'word' size is 64-bits/8 bytes
    const uint64 bitStart = ( index * _EntrySize ) - ( wordIdx * 64 );
    const uint64 bitCount = 64 - bitStart;    // Bits available in the WORD

    // Mask with only the bits of our interest 
//     value &= _EntryBits;

    // Mask out where the value will placed in the current WORD
    uint64 idxValue  = _fields[wordIdx] & _BitInv( _EntryBits << bitStart );
    _fields[wordIdx] = idxValue | ( (value & _EntryBits) << bitStart );

    if( bitCount < _EntrySize )
    {
        // Fit it into 2 consecutive WORDs
        idxValue           = _fields[wordIdx+1] & ( _InvEntryBits >> bitCount );
        _fields[wordIdx+1] = idxValue | ( ( value & _EntryBits ) >> bitCount );
    }
}

//-----------------------------------------------------------
template<size_t _EntrySize>
inline uint64 BitViewT<_EntrySize>::operator[]( uint64 index )
{
    return Get( index );
}


//-----------------------------------------------------------
template<size_t _EntrySize>
inline uint64 BitViewT<_EntrySize>::GetWordIndex( uint64 entryIndex )
{
    // 64 because our 'word' size is 64-bits/8 bytes
    return (entryIndex * _EntrySize) / 64;
}

#undef _EntryBits
#undef _InvEntryBits
#undef _BitInv