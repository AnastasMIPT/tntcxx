.. _tntcxx_api:

Tarantool C++ connector API
============================

.. // TBD -- Introduction

.. //TBD -- ToC


.. _tntcxx_api_connector:

class ``Connector``
--------------------

.. //description TBD

Methods:

* :ref:`connect() <tntcxx_api_connector_connect>`
* :ref:`wait() <tntcxx_api_connector_wait>`
* :ref:`waitAll() <tntcxx_api_connector_waitall>`
* :ref:`waitAny() <tntcxx_api_connector_waitany>`
* :ref:`close() <tntcxx_api_connector_close>`

.. _tntcxx_api_connector_connect:

..  cpp:function:: connect()

    <TBD>

    :param <param>: <TBD>


    :return: <TBD>
    :rtype: <TBD>

    **Possible errors:** <TBD>

    **Example:**

    ..  code-block:: cpp

       <code example>

.. _tntcxx_api_connector_wait:

..  cpp:function:: wait()

    <TBD>

    :param <param>: <TBD>


    :return: <TBD>
    :rtype: <TBD>

    **Possible errors:** <TBD>

    **Example:**

    ..  code-block:: cpp

       <code example>

.. _tntcxx_api_connector_waitall:

..  cpp:function:: waitAll()

    <TBD>

    :param <param>: <TBD>


    :return: <TBD>
    :rtype: <TBD>

    **Possible errors:** <TBD>

    **Example:**

    ..  code-block:: cpp

       <code example>

.. _tntcxx_api_connector_waitany:

..  cpp:function:: waitAny()

    <TBD>

    :param <param>: <TBD>


    :return: <TBD>
    :rtype: <TBD>

    **Possible errors:** <TBD>

    **Example:**

    ..  code-block:: cpp

       <code example>

.. _tntcxx_api_connector_close:

..  cpp:function:: close()

    <TBD>

    :param <param>: <TBD>


    :return: <TBD>
    :rtype: <TBD>

    **Possible errors:** <TBD>

    **Example:**

    ..  code-block:: cpp

       <code example>


.. _tntcxx_api_connection:

Class ``Connection``
--------------------

.. // class description TBD

Class signature:

..  code-block:: cpp

    template<class BUFFER, class NetProvider>
    class Connection;

Public types:

* :ref:`rid_t <tntcxx_api_connection_ridt>`

.. _tntcxx_api_connection_ridt:

..  cpp:type:: size_t rid_t

    //TBD Alias of the built-in ``size_t`` type.

Public methods:

* call()
* futureIsReady()
* getResponse()
* getError()
* reset()
* ping()
* select()
* replace()
* insert()
* update()
* upsert()
* delete()


.. _tntcxx_api_connection_call:

..  cpp:function:: template <class T> \
                    rid_t call(const std::string &func, const T &args)

    <TBD>

    :param func: <TBD>
    :param args: <TBD>

    :return: <TBD>
    :rtype: rid_t

    **Possible errors:** <TBD>

    **Example:**

    ..  code-block:: cpp

        <TBD>

.. _tntcxx_api_connection_futureisready:

..  cpp:function:: bool futureIsReady(rid_t future)

    //TBD Checks availability of a future and returns ``true`` if the future is
    available, or ``false`` otherwise.

    :param future: request ID returned by a request method, such as,
                    ``ping()``, ``select()``, ``replace()``, and so on.

    :return: ``true`` or ``false``
    :rtype: bool

    **Possible errors:** <TBD>

    **Example:**

    ..  code-block:: cpp

        <TBD>

.. _tntcxx_api_connection_getresponse:

..  cpp:function:: std::optional<Response<BUFFER>> getResponse(rid_t future)

    //TBD
    To get the response when it is ready, use the Connection::getResponse() method. It takes the request ID and returns an optional object containing the response. If the response is not ready yet, the method returns std::nullopt. Note that on each future, getResponse() can be called only once: it erases the request ID from the internal map once it is returned to a user.

    A response consists of a header and a body (response.header and response.body). Depending on success of the request execution on the server side, body may contain either runtime error(s) accessible by response.body.error_stack or data (tuples)–response.body.data. In turn, data is a vector of tuples. However, tuples are not decoded and come in the form of pointers to the start and the end of msgpacks. See the “Decoding and reading the data” section to understand how to decode tuples.

    :param future: request ID returned by a request method, such as,
                    ``ping()``, ``select()``, ``replace()``, and so on.

    :return: <TBD>
    :rtype: std::optional<Response<BUFFER>>

    **Possible errors:** <TBD>

    **Example:**

    ..  code-block:: cpp

        <TBD>

.. _tntcxx_api_connection_geterror:

..  cpp:function:: std::string& getError()

    <TBD>

    :param: none

    :return: <TBD>
    :rtype: std::string&

    **Possible errors:** <TBD>

    **Example:**

    ..  code-block:: cpp

        <TBD>

.. _tntcxx_api_connection_reset:

..  cpp:function:: void reset()

    //TBD Resets a connection after errors, that is, cleans up the error message
    and connection status.

    :param: none

    :return: none
    :rtype: none

    **Possible errors:**

    **Example:**

    ..  code-block:: cpp

        <TBD>

.. _tntcxx_api_connection_ping:

..  cpp:function:: rid_t ping()

    <TBD>

    :param: none

    :return: <TBD>
    :rtype: rid_t

    **Possible errors:** <TBD>

    **Example:**

    ..  code-block:: cpp

        <TBD>


.. _tntcxx_api_connection_space:

..  cpp:class:: Space : Connection

    //TBD A public wrapper to access the request methods in the Tarantool way,
    like, ``box.space[space_id].replace()``.

    .. _tntcxx_api_connection_select:

    ..  cpp:function:: template <class T> \
                        rid_t select(const T& key, uint32_t index_id = 0, uint32_t limit = UINT32_MAX, uint32_t offset = 0, IteratorType iterator = EQ)

        <TBD>

        :param key: <TBD>
        :param index_id: <TBD>
        :param limit: <TBD>
        :param offset: <TBD>
        :param iterator: <TBD>

        :return: <TBD>
        :rtype: rid_t

        **Possible errors:** <TBD>

        **Example:**

        ..  code-block:: cpp

            <TBD>

    .. _tntcxx_api_connection_replace:

    ..  cpp:function:: template <class T> \
                        rid_t replace(const T &tuple)

        <TBD>

        :param tuple: <TBD>

        :return: <TBD>
        :rtype: rid_t

        **Possible errors:** <TBD>

        **Example:**

        ..  code-block:: cpp

            <TBD>

    .. _tntcxx_api_connection_insert:

    ..  cpp:function:: template <class T> \
                        rid_t insert(const T &tuple)

        <TBD>

        :param tuple: <TBD>

        :return: <TBD>
        :rtype: rid_t

        **Possible errors:** <TBD>

        **Example:**

        ..  code-block:: cpp

            <TBD>

    .. _tntcxx_api_connection_update:

    ..  cpp:function:: template <class K, class T> \
                        rid_t update(const K &key, const T &tuple, uint32_t index_id = 0)

        <TBD>

        :param key: <TBD>
        :param tuple: <TBD>
        :param index_id: <TBD>

        :return: <TBD>
        :rtype: rid_t

        **Possible errors:** <TBD>

        **Example:**

        ..  code-block:: cpp

            <TBD>

    .. _tntcxx_api_connection_upsert:

    ..  cpp:function:: template <class T, class O> \
                        rid_t upsert(const T &tuple, const O &ops, uint32_t index_base = 0)

        <TBD>

        :param tuple: <TBD>
        :param ops: <TBD>
        :param index_base: <TBD>

        :return: <TBD>
        :rtype: rid_t

        **Possible errors:** <TBD>

        **Example:**

        ..  code-block:: cpp

            <TBD>

    .. _tntcxx_api_connection_delete:

    ..  cpp:function:: template <class T> \
                        rid_t delete_(const T &key, uint32_t index_id = 0)

        <TBD>

        :param key: <TBD>
        :param index_id: <TBD>

        :return: <TBD>
        :rtype: rid_t

        **Possible errors:** <TBD>

        **Example:**

        ..  code-block:: cpp

            <TBD>


.. _tntcxx_api_connection_index:

..  cpp:class:: Index : Space, Connection

    //TBD A public wrapper to access the request methods in the Tarantool way,
    like, ``box.space[sid].index[iid].select()``.

    .. _tntcxx_api_connection_select_i:

    ..  cpp:function:: template <class T> \
                        rid_t select(const T &key, uint32_t limit = UINT32_MAX, uint32_t offset = 0, IteratorType iterator = EQ)

        <TBD>

        :param key: <TBD>
        :param limit: <TBD>
        :param offset: <TBD>
        :param iterator: <TBD>

        :return: <TBD>
        :rtype: rid_t

        **Possible errors:** <TBD>

        **Example:**

        ..  code-block:: cpp

            <TBD>

    .. _tntcxx_api_connection_update_i:

    ..  cpp:function:: template <class K, class T> \
                        rid_t update(const K &key, const T &tuple)

        <TBD>

        :param key: <TBD>
        :param tuple: <TBD>

        :return: <TBD>
        :rtype: rid_t

        **Possible errors:** <TBD>

        **Example:**

        ..  code-block:: cpp

            <TBD>

    .. _tntcxx_api_connection_delete_i:

    ..  cpp:function:: template <class T> \
                        rid_t delete_(const T &key)

        <TBD>

        :param key: <TBD>

        :return: <TBD>
        :rtype: rid_t

        **Possible errors:** <TBD>

        **Example:**

        ..  code-block:: cpp

            <TBD>
