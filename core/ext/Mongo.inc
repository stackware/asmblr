<?php
/**
 * @file Mongo.inc Mongo connectivity.
 * @author Stackware, LLC
 * @package Mongo
 * @version 4.2
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 *
 * This extension is primarily composed of two classes, Mongo and Mongodb, each extending PHP's
 * native classes, MongoClient and MongoDB, respectively.  They add functionality
 * to each class but do not interfere with native functionality.
 *
 * Additional utility classes are also included.
 */
namespace asm;


/**
 * asmblr's Mongo class extends PHP's native MongoClient class and adds database aliasing
 * to avoid typos when performing database operations.  Database aliasing ensures that only
 * predefined database names are used.
 *
 * Native functionality is untouched.
 */
class Mongo extends \MongoClient implements Debuggable
{
    use Debugged;

    /**
     * @var array $Aliases
     * An array of database names that are available.
     */
    protected $Aliases = array();

    /**
     * @var array $GridFSAliases
     * An array of GridGS database names that are available.
     */
    protected $GridFSAliases = array();


    /**
     * Return a asm::MongoDB object by it's aliased name.
     *
     * @param string $DBAlias The aliased name of a database.
     * @throws Exception Unknown Mongo database alias '$DBAlias'.
     * @retval asm::MongoDB A database object.
     * @retval MongoGridFS Native grid FS class.
     *
     * @note This returns a new object each time.
     */
    public function __get( $DBAlias )
    {
        if( isset($this->Aliases[$DBAlias]) === TRUE )
        {
            if( isset($_SERVER[$this->DebugToken]) )
            {
                $BT = Debug::Backtrace();
                $BT = current(Debug::BT2Str($BT));

                Log::Log("{$DBAlias} as database {$this->Aliases[$DBAlias]} at {$BT}");
            }

            return new \asm\MongoDB($this,$this->Aliases[$DBAlias]);
        }
        else if( isset($this->GridFSAliases[$DBAlias]) === TRUE )
        {
            return parent::__get($this->GridFSAliases[$DBAlias][0])->getGridFS($this->GridFSAliases[$DBAlias][1]);
        }
        else
            throw new Exception("Unknown Mongo database alias '$DBAlias'.");
    }

    /**
     * Make a Mongo database accessible by an aliased name.
     *
     * A Mongo database cannot be accessed without first being
     * Alias()'d, even if it's to it's own name.  This is done to prevent typos
     * when trying to perform database operations.
     *
     * Existing aliases are silently overwritten.
     *
     * @param string $DBName The name of the database in Mongo.
     * @param string $DBAlias The aliased name of the database.
     * @retval asm::MongoDB Return the aliased database object.
     *
     * @note While this method may seem superflous, it's by design to
     *       prevent databases from being created because of typos.
     */
    public function Alias( $DBName,$DBAlias )
    {
        $this->Aliases[$DBAlias] = $DBName;
        return $this->__get($DBAlias);
    }

    /**
     * Make a Mongo GridFS database's accessible by an aliased name.
     *
     * A Mongo GridFS database cannot be accessed without first being
     * AliasGridFS()'d, even if it's to it's own name.
     *
     * @param string $DBAlias The alias of a Mongo database - must have already been Alias()'d.
     * @param string $Prefix The prefix that the GridFS collections will use.
     * @param string $GridFSAlias The aliased name of the GridFS database.
     * @param NULL $GridFSAlias The alias will be $Prefix.
     * @throws Exception Can't AliasGridFS() unknown Mongo database alias '$DBAlias'.
     *
     * @todo Review and confirm correctness.
     */
    public function AliasGridFS( $DBAlias,$Prefix,$GridFSAlias = NULL )
    {
        if( isset($this->Aliases[$DBAlias]) === TRUE )
            $this->GridFSAliases[$GridFSAlias===NULL?$Prefix:$GridFSAlias] = array($this->Aliases[$DBAlias],$Prefix);
        else
            throw new Exception("Can't AliasGridFS() unknown Mongo database alias '$DBAlias'.");
    }
}


/**
 * asmblr's MongoDB class extends PHP's native class of the same name and adds CRUDC operations,
 * debugging, data-mapping convenience methods, improved error handling, and other helper methods.
 *
 * Native functionality is untouched and Mongo's native exceptions are not caught.
 */
class MongoDB extends \MongoDB implements Debuggable,CRUDC
{
    use Debugged;


    /**
     * Insert a record into a collection.
     *
     * @param string $Table The collection to insert into.
     * @param array $Values Key/value array to insert.
     * @throws Exception Values must be array with collection '{$Table}'.
     * @retval mixed The _id key that was inserted.
     *
     * @note This uses MongoCollection::insert().
     */
    public function CREATE( $Table,$Values )
    {
        if( is_array($Values) === FALSE )
            throw new Exception("Values must be array with collection '{$Table}'.");

        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('CREATE',$Table);

        $this->{$Table}->insert($Values);

        return $Values['_id'];
    }

    /**
     * Read records from a collection.
     *
     * @param string $Table The collection to read from.
     * @param array $Constraint Key/value constraint array.
     * @param array $Columns Column/boolean pairs to include or exclude columns.
     * @param array $OrderBy Field names/directions (1 = ASC, -1 = DESC).  Valid MongoCollection::sort() syntax is required.
     * @param integer $Limit Limit number of records returned.
     * @retval array An array of records, or an empty array.
     *
     * @note An empty constraint is allowed - all records can be read.
     * @note This uses MongoCollection::find().
     */
    public function READ( $Table,$Constraint = NULL,$Columns = NULL,$OrderBy = array(),$Limit = 0 )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('READ',$Table);

        return iterator_to_array($this->{$Table}->find((array)$Constraint,empty($Columns)?array():$Columns)->sort($OrderBy)->limit($Limit));
    }

    /**
     * Update a single record in a collection.
     *
     * @param string $Table The collection to update.
     * @param array $Values Key/value array of new values.
     * @param array $Constraint Key/value constraint array.
     * @retval integer 0 or 1
     *
     * @note This will at most update only one record.
     * @note Depending on $Values, i.e. it doesn't use array($set=>array('Key'=>'Value')),
     *       the entire record could be replaced.  Read the Mongo documentation.
     * @note This uses MongoCollection::update() with multiple=FALSE.
     */
    public function UPDATE( $Table,$Values,$Constraint )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('UPDATE',$Table);

        return $this->{$Table}->update($Constraint,$Values,array('multiple'=>FALSE))['n'];
    }

    /**
     * Delete a single record from a collection.
     *
     * @param string $Table The collection to delete from.
     * @param array $Constraint Key/value constraint array.
     * @retval integer 0 or 1.
     *
     * @note This will at most delete only one record.
     * @note This uses MongoCollection::remove() with justOne=TRUE
     */
    public function DELETE( $Table,$Constraint )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('DELETE',$Table);

        return $this->{$Table}->remove($Constraint,array('justOne'=>TRUE))['n'];
    }

    /**
     * Counts records in a collection.
     *
     * @param string $Table The collection to count records in.
     * @param array $Constraint Key/value constraint array.
     * @retval integer The count of records.
     *
     * @note An empty constraint is allowed - all records can be counted.
     */
    public function COUNT( $Table,$Constraint = NULL )
    {
        if( isset($_SERVER[$this->DebugToken]) )
            $this->DebugCRUDC('COUNT',$Table);

        return $this->{$Table}->count($Constraint);
    }

    /**
     * Map valid fields to an array of data.
     *
     * This is useful for mapping fields from a form, for example from $_POST, to
     * a pre-defined Struct, which can then be inserted into a collection.
     *
     * Map() reads the valid fields of a ValidationReport and matches them to
     * the elements of an array.  If a field name doesn't exist in the array,
     * it is skipped.
     *
     * An associative array of field name/element name overrides can also be
     * provided to rename fields in the returned array.  If the field name
     * is mapped to NULL, or to a field that doesn't exist, it is skipped, allowing
     * certain fields to be masked.
     *
     * @param ValidationReport $VR The ValidationReport to map.
     * @param array $Src Associative array of data to map.
     * @param array $Override Array of renamed or skipped field names.
     * @retval array A consisitent associative array of field names and their values.
     *
     * @note Use this method and custom Structs for guaranteed consistency and complex nested
     *       array structures when storing data in a mongo collection.
     * @note The structure of $Override (field name=>new name) is different from
     *       that of MySQL::Map() (column name=>element name).
     * @note While the overall ValidationReport isn't checked to be valid, it
     *       likely should be before using it in this method.
     * @note A valid field is one that's TRUE.  This may include an empty field
     *       which is not required.  All others are skipped, regardless of $Override.
     *
     * @todo This may get deprecated/refactored into MongoStruct.
     */
    public function Map( \asm\ValidationReport $VR,$Src,$Override = array() )
    {
        $Dest = array();
        foreach( $VR as $K => $V )
        {
            if( $V !== TRUE )
                continue;

            // we have an override (either NULL or a renamed field name)
            if( array_key_exists($K,$Override) )
                $K2 = $Override[$K];
            // no override - field name is not renamed
            else
                $K2 = $K;

            // NULL override, skip element
            if( empty($K2) )
                continue;
            // field maps to element, map data, possibly with a renamed field name
            else if( array_key_exists($K,$Src) )
                $Dest[$K2] = $Src[$K];
            // field doesn't map to data, skip
            else
                continue;
        }

        return $Dest;
    }

    /**
     * Check whether a MongoException is caused by a duplicate key conflict.
     *
     * @param Exception $E A MongoException or child class.
     * @param NULL|string $Key Optionally test whether the duplicate is on a specific key.
     * @retval boolean TRUE if the exception is due to a duplicate key conflict, and optionally of the specified key.
     * @retval NULL The exception is a duplicate, but not of the specified key.
     *
     * @todo Why does getCode() always return 0?  So we use getMessage().
     */
    public static function IsDupeE( \Exception $E,$Key = NULL )
    {
        if( is_a($E,'MongoException') && strpos($E->getMessage(),'E11000') !== FALSE )
        {
            if( $Key === NULL )
                return TRUE;
            else
                return strpos($E->getMessage(),$Key)!==FALSE?TRUE:NULL;
        }
        else
            return FALSE;
    }

    /**
     * Print the contents of a MongoCursor.
     *
     * @param MongoCursor $M The cursor to display.
     */
    public static function DisplayCursor( \MongoCursor $M )
    {
        foreach( $M as $K => $V )
        {
            echo "\r\n\r\nKEY: $K\r\n";
            var_dump($V);
        }
    }

    /**
     * Internal method for displaying debug information about the last CRUDC query.
     *
     * @param string $Type Type of CRUDC operation.
     * @param string $Collection Collection being executed against.
     */
    protected function DebugCRUDC( $Type,$Collection )
    {
        $BT = Debug::Backtrace();
        $BT = current(Debug::BT2Str($BT));

        Log::Log("\${$this->DebugToken}::{$Type}('{$Collection}') at {$BT}",'LOG',NULL);
    }
}


/**
 * Link an asmblr Struct (PHP array) to a Mongo collection.
 *
 * This is effectively a Mongo backed array - setting or unsetting elements will
 * be applied to the corresponding document in the database, in addition to the memory resident array itself.
 *
 * @note Any element can be set/unset - it's not restricted to the document's existing structure.
 * @note Only __set()/__unset() actually hit the database - other operations just use the local array.
 * @note This should be used for straight key/value operations only - use MongoDB/MongoCollection directly for nested or complex structures.
 * @note Currently the document is linked only by _id.
 * @note Currently this cannot create a new document in a collection - it works only on existing ones.
 * @note Not all KeyValueSet methods are overriden - for example, iteration will iterate the local array, nothing in Mongo.
 *
 * @todo A MongoStruct probably isn't needed but it should be documented that a Struct that will be stored in Mongo
 *       should have a _id and CurrentTS (default 0 means never updated) in the Skel and they should be initialized in Init().
 *       Additionally, automatic updating of CurrentTS should be implemented somewhere.
 * @todo Perhaps add some Debuggable awareness like MongoDB has.
 */
class MongoKVS extends \asm\KeyValueSet
{
    protected $MDC;
    protected $CollectionName;


    /**
     * Create a new MongoKVS object.
     *
     * $Constraint is taken as a findOne() compatible query to find a single record which will be managed by the object.
     *
     * If $Constraint isn't supplied, the object will be "empty" and count($obj) will return 0.
     *
     * @param MongoCollection $MDC The Mongo collection the array is stored in.
     * @param array $Constraint findOne() compatible lookup constraint.
     *
     * @note Unlike it's base class, this will NOT reference the array passed in.  Use Export() to access the updated array.
     * @note Link() uses references for linking an existing array (document).
     * @note If the constraint returns 0 records this will succeed but be "empty."  getter/setter calls will throw exceptions in this case.
     * @note count($obj) will return the number of fields if a record is found.  This class doesn't handle multiple records.
     */
    public function __construct( \MongoCollection $MDC,$Constraint = array() )
    {
        $this->MDC = $MDC;
        $this->CollectionName = $this->MDC->getName();

        if( !empty($Constraint) )
        {
            $R = $this->MDC->findOne($Constraint);
            if( !empty($R) )
                $this->Link($R);
        }
    }

    /**
     * Key/value setter using property overloading and MongoDB as storage.
     *
     * If $Key exists, it's value is silently overwritten.
     *
     * @param scalar $Key The element's key to set (string or integer).
     * @param mixed $Value The element's value to set.
     * @retval boolean TRUE if $Key was found and set.
     * @throws Exception KVS with collection '{$this->CollectionName}' is empty.
     *
     * @note This is not nested document/dot-notation aware - the entire value is set for the key.
     *
     * @todo Add versioning and CurrentTS awareness which needs to consider MongoDate usage vs UNIX timestamp int.
     * @todo Similarly, could add another method for setting multiple fields only if changed, and implement
     *       an optimistic locking pattern - general usage of sync'ing local values with DB values would be huge.
     */
    public function __set( $Key,$Value )
    {
        if( empty($this->KV['_id']) )
            throw new Exception("KVS with collection '{$this->CollectionName}' is empty.");

        // CurrentTS/versioning isn't implemented as a constraint: 'CurrentTS'=>$Haystack['CurrentTS'])
        $R = $this->MDC->update(array('_id'=>$this->KV['_id']),
                                array('$set'=>array($Key=>$Value)),array('multiple'=>FALSE));

        if( empty($R['err']) && $R['n'] >= 1 )
        {
            $this->KV[$Key] = $Value;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    /**
     * Unset a key/value pair using property overloading and MongoDB as storage.
     *
     * @param scalar $Key The element's key to unset (string or integer).
     * @retval boolean TRUE if $Key was found and unset.
     * @throws Exception KVS with collection '{$this->CollectionName}' is empty.
     *
     * @note This is not nested document/dot-notation aware - the entire value is set for the key.
     *
     * @see __set() for versioning related and other comments.
     */
    public function __unset( $Key )
    {
        if( empty($this->KV['_id']) )
            throw new Exception("KVS with collection '{$this->CollectionName}' is empty.");

        if( $this->__isset($Key) )
        {
            $R = $this->MDC->update(array('_id'=>$this->KV['_id']),
                    array('$unset'=>array($Key=>'')),array('multiple'=>FALSE));

            if( empty($R['err']) && $R['n'] >= 1 )
            {
                unset($this->KV[$Key]);
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }

    /**
     * Link to a Mongo document (array) using _id.
     *
     * If a document is already linked it is silently discarded.
     *
     * @param $ArrayRef A reference to an array.
     * @throws Exception Linked document doesn't contain _id.
     *
     * @todo Add PrimaryKey parameter so it's not only _id.
     */
    public function Link( &$ArrayRef )
    {
        if( empty($ArrayRef['_id']) )
            throw new Exception('Linked document doesn\'t contain _id.');

        $this->KV = &$ArrayRef;
    }

    /**
     * Retrieve the timestamp from the current document, using the _id.
     *
     * This will be the creation timestamp.
     *
     * @param string $Format Optional format for date/time.  Otherwise the UNIX timestamp integer is returned.
     * @throws Exception KVS with collection '{$this->CollectionName}' is empty.
     *
     * @retval int The UNIX timestamp.
     * @retval string The formatted date/time.
     *
     * @note This assumes the _id element is a MongoId.
     */
    public function TS( $Format = '' )
    {
        if( empty($this->KV['_id']) )
            throw new Exception("KVS with collection '{$this->CollectionName}' is empty.");

        if( empty($Format) )
            return $this->KV['_id'];
        else
            return date($Format,$this->KV['_id']->getTimestamp());
    }
}




/**
 * Manage an element of an asmblr Struct (PHP array) as an ordered list in it's Mongo collection.
 *
 * This manages a sub-document $ListElement within an single record as an ordered list, implementing CRUDC for the list.
 *
 * @note Currently the document is linked only by _id.
 *
 * @todo Perhaps add some Debuggable awareness like MongoDB has.
 */
class MongoList
// implements CRUDC
{
    protected $MDC;
    protected $Record;
    protected $ListElement = 'MongoList';


    /**
     * Create a new MongoList object.
     *
     * $Constraint is taken as a findOne() compatible query to find a single record which will be managed by the object.
     *
     * If $Constraint doesn't produce one record, an exception is thrown.
     *
     * $ListElement is the element within the record that will be managed as a list.  It is created or overwritten.
     *
     * @param MongoCollection $MDC The Mongo collection the array is stored in.
     * @param array $Constraint findOne() compatible lookup constraint.
     * @param string $ListElement The name of the element to treat as an ordered list.
     * @param NULL $ListElement Default of 'MongoList' element name will be used.
     * @throws Exception Linked document doesn't contain _id.
     */
    public function __construct( \MongoCollection $MDC,$Constraint,$ListElement = NULL )
    {
        $this->MDC = $MDC;

        $R = $this->MDC->findOne($Constraint);
        if( !empty($R) )
            $this->Link($R);
        else
            throw new Exception('Constraint produced no records.');

        if( !empty($ListElement) )
            $this->ListElement = $ListElement;
    }


    /**
     * Create (insert) an item into the list.
     *
     * @param mixed $Item The item to insert.
     * @param null $Position Append the item to the list (default).
     * @param integer Position to insert to, counted from 0.
     * @throws Exception Invalid Position - must be an integer or NULL.
     *
     * @note Note the difference between a $Position of NULL and 0.
     * @note The item is always inserted as it's own full element within the list.
     */
    public function CREATE( $Item,$Position = NULL )
    {
        if( $Position === NULL )
        {
            $this->MDC->update(array('_id'=>$this->Record['_id']),
                               array('$push'=>array($this->ListElement=>$Item)));
        }
        else if( is_int($Position) )
        {
            $this->MDC->update(array('_id'=>$this->Record['_id']),
                               array('$push'=>array($this->ListElement=>array('$each'=>array($Item),'$position'=>$Position))));
        }
        else
        {
            throw new Exception('Invalid Position - must be an integer or NULL.');
        }
    }

    /**
     * Read one or more items.
     *
     * @param NULL $Start Read all items (default).
     * @param integer $Start The item position to read, counted from 0.
     * @param integer $Length Number of items to read from $Start if $Start is not NULL.
     * @throws Exception Invalid Start or Length - must be an integer or NULL.
     * @retval array List of one or more item.
     *
     * @note Even if a Start/Length is provided, the returned list of items will be 0 indexed.
     *
     * @todo May want to implement MongoCollection::aggregate to order/filter the items (rather than just a slice).
     * @todo _id=>1 prevents the rest of the document from being returned; might be a read option?
     */
    public function READ( $Start = NULL,$Length = 1 )
    {
        if( $Start === NULL )
            $Slice = array('_id'=>1,$this->ListElement=>1);
        else if( is_integer($Start) && is_integer($Length) )
            $Slice = array('_id'=>1,$this->ListElement=>array('$slice'=>array($Start,$Length)));
        else
            throw new Exception('Invalid Start or Length - must be an integer or NULL.');

        $R = $this->MDC->findOne(array('_id'=>$this->Record['_id']),$Slice);

        // if we want to single item only (not as a part of a list)
        // return $Slice[$this->ListElement]['$slice'][1]===1?current($R[$this->ListElement]):$R[$this->ListElement];

        if( !empty($R[$this->ListElement]) )
            return $R[$this->ListElement];
        else
            return array();
    }

    /**
     * Shorthand method for MongoList::READ()
     */
    public function __invoke( $Start = NULL,$Length = 1 )
    {
        return $this->READ($Start,$Length);
    }

    /**
     * Update a single item.
     *
     * @param array $Item The new value of the item.
     * @param integer $Position Position of item to update, counted from 0.
     * @throws Exception Invalid Position - must be an integer.
     *
     * @note This will completely overwrite the item with the new value.
     * @note If $Position is out of bounds, Mongo will pad the list with NULL elements.
     */
    public function UPDATE( $Item,$Position )
    {
        if( !is_int($Position) )
            throw new Exception('Invalid Position - must be an integer.');

        $this->MDC->update(array('_id'=>$this->Record['_id']),
                           array('$set'=>array($this->ListElement.'.'.$Position=>$Item)));
    }

    /**
     * Delete a single item.
     *
     * @param integer $Position Position of item to delete, counted from 0.
     * @throws Exception Invalid Position - must be an integer.
     */
    public function DELETE( $Position )
    {
        if( !is_int($Position) )
            throw new Exception('Invalid Position - must be an integer.');

        $this->MDC->findAndModify(array('_id'=>$this->Record['_id']),array('$unset'=>array($this->ListElement.'.'.$Position=>TRUE)));
        $this->MDC->update(array('_id'=>$this->Record['_id']),array('$pull'=>array($this->ListElement=>NULL)));
    }

    /**
     * Counts items in a list.
     *
     * @retval integer The count of list items.
     */
    public function COUNT()
    {
        return count($this->READ());
    }

    /**
     * Link to a Mongo document (array) using _id.
     *
     * If a document is already linked it is silently discarded.
     *
     * @param $ArrayRef A reference to an array.
     * @throws Exception Linked document doesn't contain _id.
     *
     * @note The memory resident array isn't kept in sync with what's updated in Mongo - always use READ().
     *
     * @todo Add PrimaryKey parameter so it's not only _id.
     */
    public function Link( &$ArrayRef )
    {
        if( empty($ArrayRef['_id']) )
            throw new Exception('Linked document doesn\'t contain _id.');

        $this->Record = &$ArrayRef;
    }

    /**
     * Returns a reference to the record used for managing the list.
     *
     * @note The ListElement in this local array isn't kept updated and shouldn't be used - always use READ().
     */
    public function &GetRecord()
    {
        return $this->Record;
    }

}

